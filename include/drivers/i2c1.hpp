#pragma once

#include <cstddef>
#include <cstdint>

namespace drivers::i2c1 {

enum class ProbeResult : std::uint8_t {
  acknowledged,
  bus_busy_timeout,
  start_timeout,
  not_acknowledged,
  response_timeout
};

enum class ReadResult : std::uint8_t {
  success,
  invalid_argument,
  bus_busy_timeout,
  start_timeout,
  address_not_acknowledged,
  response_timeout,
  transmit_timeout,
  receive_timeout
};

enum class WriteResult : std::uint8_t {
  success,
  bus_busy_timeout,
  start_timeout,
  address_not_acknowledged,
  response_timeout,
  transmit_timeout
};

void initialize();

ProbeResult probe(std::uint8_t address);

ReadResult read_register(std::uint8_t address, std::uint8_t register_address,
                         std::uint8_t &value);

ReadResult read_registers(std::uint8_t address, std::uint8_t start_register,
                          std::uint8_t *buffer, std::size_t length);

WriteResult write_register(std::uint8_t address, std::uint8_t register_address,
                           std::uint8_t value);

std::uint32_t last_sr1();
std::uint32_t last_sr2();

} // namespace drivers::i2c1