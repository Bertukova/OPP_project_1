#include <array>
#include <cstddef>
#include <chrono>
#include <condition_variable>
#include <iostream>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <string_view>
#include <thread>

namespace {

// индексы компонентов
// перечисление, пользовательский тип Ingredient с конечным набором именованных констант
// имена констант не попадают во внешнюю область видимости (нужно писать Ingredient::kTobacco)
// нет неявного преобразования к целочисленным типам (в int придётся приводить через static_cast)
// три константы этого типа и их целочисленные значения
enum class Ingredient { kTobacco = 0, kPaper = 1, kMatches = 2 }; // префикс k в именах обозначает константу

// функция возвращает целочисленный индекс для значения строгого перечисления Ingredient (enum class)
// явное преобразование к целочисленному типу
// std::size_t - тип результата, беззнаковы целочисленный тип
constexpr std::size_t IngredientIndex(Ingredient ingredient) {
  return static_cast<std::size_t>(ingredient);
}

// ф-ия нужна для того, чтобы быстро получить имя по индексу
// названия ингридиентов в им. падеже
constexpr std::array<std::string_view, 3> kIngredientNames{
    "табак", "бумага", "спички"};

// то же самое, но в тв. падеже
constexpr std::array<std::string_view, 3> kSmokerResources{
    "табаком", "бумагой", "спичками"};

// списко всех видов курильщиков
constexpr std::array<Ingredient, 3> kAllSmokers{
    Ingredient::kTobacco, Ingredient::kPaper, Ingredient::kMatches};

// кол-во типов курильщиков
constexpr std::size_t kSmokerCount = kAllSmokers.size();

// пара компонентов, который кладет на стол посредник
// другими словами, два компонента, которых не хватает для конкретного курильщика
constexpr std::array<std::array<Ingredient, 2>, 3> kComponentsForSmoker{{
    {Ingredient::kPaper, Ingredient::kMatches},
    {Ingredient::kTobacco, Ingredient::kMatches},
    {Ingredient::kTobacco, Ingredient::kPaper},
}};

// ф-ия возвращает строковое имя ингридинета по индексу (по значению перечисление Ingredient)
std::string_view IngredientToString(Ingredient ingredient) {
  return kIngredientNames[IngredientIndex(ingredient)];
}

// ф-ия возвращает тип курильщака в формате:
// курильщик с ...
std::string SmokerLabel(Ingredient ingredient) {
  const auto index = IngredientIndex(ingredient);
  return std::string{"курильщик с "} + std::string{kSmokerResources[index]};
}

// ф-ия возвращает недостающие компненты для конкретного курильщика
// курильщику недостает *пары компонентов*
std::array<Ingredient, 2> ComponentsFor(Ingredient smoker) {
  return kComponentsForSmoker[IngredientIndex(smoker)];
}

// Мьютекс - "замок"
// Когда один поток захватил мьютекс, все остальные, кто тоже хочет его захватить ждут, пока первый не отпустит
// Код внутри «закрытого участка» выполняется строго по одному потоку за раз
// Нужен для того, чтобы защитить общие данные от одновременной записи/чтения несколькими потоками
// Иначе получается гонка данными и программа ведет себя непредсказуемо

// ф-ия печатает каждое сообщение целиком и гарантирует, что без перемешивания логов от разных потоков
// std::mutex& io_mutex — общий замок, один на всех, передаётся по ссылке (чтобы все пользовались одним и тем же замком)
// std::lock_guard<std::mutex> lock(io_mutex); — «захватить замок» на время этой функции. При выходе из функции замок автоматически отпустится
void PrintMessage(std::mutex& io_mutex, const std::string& message) {
  std::lock_guard<std::mutex> lock(io_mutex);
  std::cout << message << std::endl;
}

// 4 потока круглого стола, или не круглого
// стол, за которым сидят 3 курилбщика и 1 посредник
class SmokingTable {
 public:
 // метод посредника
 // он кладет на стол два компонента, но делает это только тогда, когда прошлый этап/раунд завершен
 // на вход подаются две детали, которые порседник хочет выложить
 void place(Ingredient first, Ingredient second) {
    std::unique_lock<std::mutex> lock(mutex_); // замок
    table_cv_.wait(lock, [this] { // ждем, пока можно выложить два компонента, если условие истинно => не засыпаем
        return finished_ || (!items_.has_value() && !smoker_busy_); // если ложно, отпускаем mutex_ и засыпаем, кто-то другой сможет изменить состояние и разбудить нас
        // не вылетает из функции», а блокирует выполнение до тех пор, пока предикат не станет true
        // после этого код продолжается на следующей строке после wait
    });
    if (finished_) { // прверяем на завершение процесс, если true, то выходим и ничего не выкладываем
        return;
    }
    items_.emplace(std::array<Ingredient, 2>{first, second}); // выкладываем на стол пару компонентов
    smoker_cv_.notify_all(); // будим всех курильщиков, чтоб они могли компоненты забрать себе и начать курить
 }

 // метод курильщика
 // блокирует и ждет, пока на столе появится его пара компонентов, либо пока не придет сигнал завершения
 // если текущая пара - его,то он начинает курить, то есть он занят, стол им очищен и новый раунд начался
 bool startSmoking(Ingredient owned) {
    std::unique_lock<std::mutex> lock(mutex_);
    smoker_cv_.wait(lock, [this, owned] {
      return finished_ || (items_.has_value() && Needs(owned, *items_));
    });
    if (finished_) {
      return false; // если true, то курить не начинаем
    }
    smoker_busy_ = true;
    items_.reset();
    lock.unlock(); // отпускаем мьютекс до уведомления
    table_cv_.notify_all(); // стол пуст, курильщик занят, новый раунд
    return true; // сигнал о начале раунда
  }
  
  // курильщик докурил
  void finishSmoking() {
    std::lock_guard<std::mutex> lock(mutex_); // не нужно ничего ждать, не нужно вручную делать unlock
    smoker_busy_ = false;
    table_cv_.notify_all(); // будем всех ожидающих, прежде всего посредника, кот-ый либо в waitForRoundEnd(), либо в place() ждёт конца предыдущего раунда
  }
  
  // посредник выложил два компонента, вызывает данную функцию и ждет конца текущего раунда перед тем, как объявить о завершении и переходить к следующему
  void waitForRoundEnd() {
    std::unique_lock<std::mutex> lock(mutex_);
    table_cv_.wait(lock, [this] {
      return finished_ || (!items_.has_value() && !smoker_busy_);
    });
  }
  
  // посредник сворачивает происходящее, все расходятся
  void finish() {
    std::lock_guard<std::mutex> lock(mutex_);
    finished_ = true;
    items_.reset();
    smoker_busy_ = false;
    table_cv_.notify_all();
    smoker_cv_.notify_all();
  }
 
 // подходит ли пара на столе курильщику
 private:
  static bool Needs(Ingredient owned, const std::array<Ingredient, 2>& items) {
    return items[0] != owned && items[1] != owned;
  }

  std::mutex mutex_{}; // мьютекс: когда он захватывает поток, другие потоки ждут, пока он не совободится; любой доступ к общему состоянию стола выполняется под этим замком
  // / условная переменная, своеобразный механизм, кот-ый успыляет поток до наступления опр. условия, а затем будит его сигналом
  std::condition_variable table_cv_{}; // посредник ждет, пока курильщик накурится, то есть стол опустеет
  std::condition_variable smoker_cv_{}; // посредник выложил пару компонентов и ему нужно пнуть курильщиков, чтоб кто-то курить начал
  std::optional<std::array<Ingredient, 2>> items_{}; // либо стол пуст, либо на столе лежит пара компонентов
  bool smoker_busy_{false}; // курит ли кто-то из курильщиков сейчас?
  bool finished_{false}; // пора сворачиваться
};



}  // namespace

int main() {
  SmokingTable table; 
  std::mutex io_mutex; // мьютекс для логов
  constexpr int kTotalRounds = 12; // кол-во раундов, которые проведет посредник
  const auto rolling_duration = std::chrono::milliseconds(150); // время на скручивание сигареты
  const auto smoking_duration = std::chrono::milliseconds(300); // время на курение

  // счетчик сигарет по каждому из курильщиков 
  std::array<int, kSmokerCount> smoked_count{}; // {} - value-инициализация всех элементов, т.е. каждый элемент у нас 0, а не просто мусорное значение
  
  // поток курильщика
  // лямбда-функция 
  // [&] - захват по ссылке всего, что будет использовано из внешней области
  // Ingredient ingredient - что именно у этого курильщика своё, т.е. тип курильщика
  // int& counter — ссылка на уже существующий счётчик этого курильщика
  auto smoker_task = [&](Ingredient ingredient, int& counter) {  
    const auto components = ComponentsFor(ingredient); // 2 недостающих компонента для рассматриваемого курильщика
    while (true) { // поток курильщика
      if (!table.startSmoking(ingredient)) {
        break; // выход из цикла, если у нас курит другой курильщик
      }

      ++counter;

      {
        std::string message = SmokerLabel(ingredient) +
                               " забирает " +
                               std::string{IngredientToString(components[0])} +
                               " и " +
                               std::string{IngredientToString(components[1])} +
                               ".";
        PrintMessage(io_mutex, message);
      }

      std::this_thread::sleep_for(rolling_duration); // sleep_for() — спать относительное время

      {
        std::string message = SmokerLabel(ingredient) +
                               " скрутил сигарету #" +
                               std::to_string(counter) + ".";
        PrintMessage(io_mutex, message);
      }

      std::this_thread::sleep_for(smoking_duration);

      {
        std::string message = SmokerLabel(ingredient) +
                               " докурил сигарету #" +
                               std::to_string(counter) + ".";
        PrintMessage(io_mutex, message);
      }
      
      table.finishSmoking();
    }

    PrintMessage(io_mutex, SmokerLabel(ingredient) + " завершает работу.");
  };

  // поток посредника
  auto agent_task = [&](int total_rounds) {
    std::mt19937 rng(std::random_device{}()); // генератор псевдослучайных чисел
    std::uniform_int_distribution<int> dist(
        0, static_cast<int>(kAllSmokers.size()) - 1); // равномерное распределение в диапазоне от [0; 2]

    for (int round = 1; round <= total_rounds; ++round) {
      const Ingredient smoker_with_supply =
          kAllSmokers[static_cast<std::size_t>(dist(rng))]; // случайно выбираем какому курильщику будет подходить след. пара компонентов
      const auto components = ComponentsFor(smoker_with_supply); // та самая пара компонентов
      
      {
        std::string message =
            "Посредник выкладывает " +
            std::string{IngredientToString(components[0])} + " и " +
            std::string{IngredientToString(components[1])} + " для " +
            SmokerLabel(smoker_with_supply) +
            ". Раунд #" + std::to_string(round) + ".";
        PrintMessage(io_mutex, message);
      }

      table.place(components[0], components[1]);

      table.waitForRoundEnd();

      {
        std::string message = "Раунд #" + std::to_string(round) +
                               " завершен.";
        PrintMessage(io_mutex, message);
      }
    }

    table.finish();
    PrintMessage(io_mutex, "Посредник завершает работу.");
  };

  return 0;
}