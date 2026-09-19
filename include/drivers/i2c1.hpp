#pragma once

#include "drivers/i2c.hpp"

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

void initialize();

ProbeResult probe(std::uint8_t address);

i2c::ReadResult read_register(std::uint8_t address,
                              std::uint8_t register_address,
                              std::uint8_t &value);

i2c::ReadResult read_registers(std::uint8_t address,
                               std::uint8_t start_register,
                               std::uint8_t *buffer, std::size_t length);

i2c::WriteResult write_register(std::uint8_t address,
                                std::uint8_t register_address,
                                std::uint8_t value);

std::uint32_t last_sr1();
std::uint32_t last_sr2();
std::uint32_t recovery_count();

} // namespace drivers::i2c1
