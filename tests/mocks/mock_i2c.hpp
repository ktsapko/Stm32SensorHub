#pragma once

#include "drivers/i2c.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace tests::mocks {

class MockI2c {
public:
  MockI2c() {
    registers.fill(0U);
    read_result = drivers::i2c::ReadResult::success;
  }

  void set_register(const std::uint8_t address,
                    const std::uint8_t register_address,
                    const std::uint8_t value) {
    registers[index(address, register_address)] = value;
  }

  const drivers::i2c::I2cBus &bus() const { return mock_bus; }

  void set_read_result(const drivers::i2c::ReadResult result) {
    read_result = result;
  }

  std::uint8_t get_register(const std::uint8_t address,
                            const std::uint8_t register_address) const {
    return registers[index(address, register_address)];
  }

private:
  static constexpr std::size_t registers_per_device = 256U;
  static constexpr std::size_t device_count = 128U;

  static std::size_t index(const std::uint8_t address,
                           const std::uint8_t register_address) {
    return static_cast<std::size_t>(address) * registers_per_device +
           register_address;
  }

  static drivers::i2c::ReadResult
  read_register(const std::uint8_t address, const std::uint8_t register_address,
                std::uint8_t &value) {
    if (read_result != drivers::i2c::ReadResult::success) {
      return read_result;
    }

    value = registers[index(address, register_address)];
    return drivers::i2c::ReadResult::success;
  }

  static drivers::i2c::ReadResult
  read_registers(const std::uint8_t address, const std::uint8_t start_register,
                 std::uint8_t *buffer, const std::size_t length) {
    if (read_result != drivers::i2c::ReadResult::success) {
      return read_result;
    }
    for (std::size_t i = 0; i < length; ++i) {
      buffer[i] = registers[index(
          address, static_cast<std::uint8_t>(start_register + i))];
    }

    return drivers::i2c::ReadResult::success;
  }

  static drivers::i2c::WriteResult
  write_register(const std::uint8_t address,
                 const std::uint8_t register_address,
                 const std::uint8_t value) {
    registers[index(address, register_address)] = value;
    return drivers::i2c::WriteResult::success;
  }

  inline static std::array<std::uint8_t, device_count * registers_per_device>
      registers{};

  inline static constexpr drivers::i2c::I2cBus mock_bus{
      .read_register = read_register,
      .read_registers = read_registers,
      .write_register = write_register,
  };

  inline static drivers::i2c::ReadResult read_result =
      drivers::i2c::ReadResult::success;
};

} // namespace tests::mocks
