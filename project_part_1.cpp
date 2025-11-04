#include <array>
#include <chrono>
#include <random>
#include <thread>
#include <locale.h>

#include "smoking_types.hpp"
#include "smoking_io.hpp"
#include "smoking_table.hpp"

int main() {
  setlocale(LC_ALL, "Russian"); // для корректного вывода русских сообщений в консоль на Windows
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

  std::array<std::thread, kSmokerCount> smokers{}; // массив потоков курильщиков
  for (std::size_t i = 0; i < smokers.size(); ++i) { // запуск трех потоков курильщиков
    smoked_count[i] = 0;
    smokers[i] = std::thread(smoker_task, kAllSmokers[i], // конструируем временный объект потока и запускаем в нем лямбда-функцию
                             std::ref(smoked_count[i]));
  }

  std::thread agent(agent_task, kTotalRounds);

  agent.join(); // текущий поток (main) ждёт, пока поток agent (посредник) полностью завершится
  for (auto& smoker : smokers) {
    if (smoker.joinable()) { // владеет ли этот объект std::thread smoker действующим потоком?
      smoker.join(); // ждем завершения конркетного курильщика, который курит 
    }
  }

  PrintMessage(io_mutex, "Итоговая статистика:");
  for (std::size_t i = 0; i < smoked_count.size(); ++i) {
    std::string message = SmokerLabel(kAllSmokers[i]) + " выкурил " +
                          std::to_string(smoked_count[i]) + " сигарет.";
    PrintMessage(io_mutex, message);
  }

  return 0;
}