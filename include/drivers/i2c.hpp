#pragma once

#include <cstddef>
#include <cstdint>

namespace drivers::i2c {

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

struct I2cBus {
  ReadResult (*read_register)(std::uint8_t, std::uint8_t, std::uint8_t &);
  ReadResult (*read_registers)(std::uint8_t, std::uint8_t, std::uint8_t *,
                               std::size_t);
  WriteResult (*write_register)(std::uint8_t, std::uint8_t, std::uint8_t);
};

} // namespace drivers::i2c
