#pragma once

#include <cstddef>
#include <cstdint>

namespace drivers::usart2 {

void initialize();

bool write_byte(char byte);
std::size_t write(const char* text);

[[nodiscard]] std::uint32_t dropped_tx_bytes();

} // namespace drivers::usart2
