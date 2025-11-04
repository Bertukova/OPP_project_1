#pragma once

#include <array>
#include <condition_variable>
#include <mutex>
#include <optional>

#include "smoking_types.hpp"

namespace {

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

} // namespace
