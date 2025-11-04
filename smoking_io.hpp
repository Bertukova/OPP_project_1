#pragma once

#include <iostream>
#include <mutex>
#include <string>

namespace {

// ф-ия печатает каждое сообщение целиком и гарантирует, что без перемешивания логов от разных потоков
// std::mutex& io_mutex — общий замок, один на всех, передаётся по ссылке (чтобы все пользовались одним и тем же замком)
// std::lock_guard<std::mutex> lock(io_mutex); — «захватить замок» на время этой функции. При выходе из функции замок автоматически отпустится
void PrintMessage(std::mutex& io_mutex, const std::string& message) {
  std::lock_guard<std::mutex> lock(io_mutex);
  std::cout << message << std::endl;
}

} // namespace
