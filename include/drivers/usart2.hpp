#pragma once

#include <cstddef>
#include <cstdint>

namespace drivers::usart2 {

bool initialize();

bool write_byte(char byte);
std::size_t write(const char *text);

[[nodiscard]] std::uint32_t dropped_tx_bytes();
bool read_byte(std::uint8_t &byte);

[[nodiscard]] std::uint32_t dropped_rx_bytes();
[[nodiscard]] std::uint32_t hardware_rx_overruns();

struct DmaTxDiagnostics {
  std::uint32_t transfer_errors;
  std::uint32_t direct_mode_errors;
  std::uint32_t fifo_errors;
  std::uint32_t completed_transfers;
  std::uint32_t recovery_attempts;
  std::uint32_t recovery_failures;
  std::uint32_t uncertain_bytes;
};

[[nodiscard]] DmaTxDiagnostics dma_tx_diagnostics();
void service_tx();

} // namespace drivers::usart2
