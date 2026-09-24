#pragma once

#include <cstddef>
#include <cstdint>

namespace drivers::usart2 {

void initialize();

bool write_byte(char byte);
std::size_t write(const char *text);

[[nodiscard]] std::uint32_t dropped_tx_bytes();
bool read_byte(std::uint8_t &byte);

[[nodiscard]] std::uint32_t dropped_rx_bytes();
[[nodiscard]] std::uint32_t hardware_rx_overruns();

} // namespace drivers::usart2
