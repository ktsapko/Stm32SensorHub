#include "drivers/bmp280.hpp"

#include "drivers/i2c1.hpp"

#include <cstddef>
#include <cstdint>

namespace drivers::bmp280 {
namespace {

constexpr std::uint8_t address = 0x76U;
constexpr std::uint8_t chip_id_register = 0xD0U;
constexpr std::uint8_t control_measurement_register = 0xF4U;
constexpr std::uint8_t normal_mode_configuration =
    0x27U; // Temperature and pressure oversampling x1, normal mode

constexpr std::uint8_t calibration_data_start_register = 0x88U;
constexpr std::size_t calibration_size = 24U;

constexpr std::uint32_t bits_per_byte = 8U;
constexpr std::uint16_t signed_word_threshold = 0x8000U;
constexpr std::int32_t signed_word_modulus = 65'536;

std::uint16_t decode_unsigned_word(const std::uint8_t low,
                                   const std::uint8_t high) {
  return static_cast<std::uint16_t>(
      static_cast<std::uint16_t>(low) |
      (static_cast<std::uint16_t>(high) << bits_per_byte));
}

std::int16_t decode_signed_word(const std::uint8_t low,
                                const std::uint8_t high) {
  const std::uint16_t raw_value = decode_unsigned_word(low, high);

  const std::int32_t signed_value =
      static_cast<std::int32_t>(raw_value) -
      (raw_value >= signed_word_threshold ? signed_word_modulus : 0);

  return static_cast<std::int16_t>(signed_value);
}

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

bool read_calibration(CalibrationData &calibration) {
  std::uint8_t bytes[calibration_size]{};

  const auto result = drivers::i2c1::read_registers(
      address, calibration_data_start_register, bytes, calibration_size);

  if (result != drivers::i2c1::ReadResult::success) {
    return false;
  }

  calibration.dig_T1 = decode_unsigned_word(bytes[0], bytes[1]);
  calibration.dig_T2 = decode_signed_word(bytes[2], bytes[3]);
  calibration.dig_T3 = decode_signed_word(bytes[4], bytes[5]);

  calibration.dig_P1 = decode_unsigned_word(bytes[6], bytes[7]);
  calibration.dig_P2 = decode_signed_word(bytes[8], bytes[9]);
  calibration.dig_P3 = decode_signed_word(bytes[10], bytes[11]);
  calibration.dig_P4 = decode_signed_word(bytes[12], bytes[13]);
  calibration.dig_P5 = decode_signed_word(bytes[14], bytes[15]);
  calibration.dig_P6 = decode_signed_word(bytes[16], bytes[17]);
  calibration.dig_P7 = decode_signed_word(bytes[18], bytes[19]);
  calibration.dig_P8 = decode_signed_word(bytes[20], bytes[21]);
  calibration.dig_P9 = decode_signed_word(bytes[22], bytes[23]);

  return true;
}
} // namespace drivers::bmp280