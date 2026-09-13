#include "drivers/bmp280.hpp"

#include "drivers/i2c1.hpp"

#include <cstdint>

namespace drivers::bmp280 {
namespace {

constexpr std::uint8_t address = 0x76U;
constexpr std::uint8_t chip_id_register = 0xD0U;
constexpr std::uint8_t control_measurement_register = 0xF4U;
constexpr std::uint8_t normal_mode_configuration =
    0x27U; // Temperature and pressure oversampling x1, normal mode

} // namespace

bool read_chip_id(std::uint8_t &chip_id) {
  return drivers::i2c1::read_register(address, chip_id_register, chip_id) ==
         drivers::i2c1::ReadResult::success;
}
bool initialize() {
  const auto write_result = drivers::i2c1::write_register(
      address, control_measurement_register, normal_mode_configuration);

  if (write_result != drivers::i2c1::WriteResult::success) {
    return false;
  }

  std::uint8_t configuration = 0U;
  const auto read_result = drivers::i2c1::read_register(
      address, control_measurement_register, configuration);
  return read_result == drivers::i2c1::ReadResult::success &&
         configuration == normal_mode_configuration;
}
} // namespace drivers::bmp280