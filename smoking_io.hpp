#pragma once

#include <iostream>
#include <mutex>
#include <string>

namespace {

// ф-и€ печатает каждое сообщение целиком и гарантирует, что без перемешивани€ логов от разных потоков
// std::mutex& io_mutex Ч общий замок, один на всех, передаЄтс€ по ссылке (чтобы все пользовались одним и тем же замком)
// std::lock_guard<std::mutex> lock(io_mutex); Ч Ђзахватить замокї на врем€ этой функции. ѕри выходе из функции замок автоматически отпуститс€
void PrintMessage(std::mutex& io_mutex, const std::string& message) {
  std::lock_guard<std::mutex> lock(io_mutex);
  std::cout << message << std::endl;
}

} // namespace