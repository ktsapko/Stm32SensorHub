#include "drivers/bmp280.hpp"

#include "sensors/decoding.hpp"

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

constexpr std::uint8_t measurement_start_register = 0xF7U;
constexpr std::size_t measurement_size = 6U;

constexpr float pascals_per_hectopascal = 100.0F;

CalibrationData calibration_data{};
bool calibration_available = false;

const i2c::I2cBus *i2c_bus = nullptr;

} // namespace

bool read_chip_id(std::uint8_t &chip_id) {
  return i2c_bus->read_register(address, chip_id_register, chip_id) ==
         drivers::i2c::ReadResult::success;
}

bool initialize() {
  calibration_available = false;

  const auto write_result = i2c_bus->write_register(
      address, control_measurement_register, normal_mode_configuration);

  if (write_result != drivers::i2c::WriteResult::success) {
    return false;
  }

  std::uint8_t configuration = 0U;

  const auto read_result = i2c_bus->read_register(
      address, control_measurement_register, configuration);

  if (read_result != drivers::i2c::ReadResult::success ||
      configuration != normal_mode_configuration) {
    return false;
  }

  calibration_available = read_calibration(calibration_data);
  return calibration_available;
}

bool read_calibration(CalibrationData &calibration) {
  std::uint8_t bytes[calibration_size]{};

  const auto result = i2c_bus->read_registers(
      address, calibration_data_start_register, bytes, calibration_size);

  if (result != drivers::i2c::ReadResult::success) {
    return false;
  }

  calibration.dig_T1 =
      sensors::decoding::decode_unsigned_word_le(bytes[0], bytes[1]);
  calibration.dig_T2 =
      sensors::decoding::decode_signed_word_le(bytes[2], bytes[3]);
  calibration.dig_T3 =
      sensors::decoding::decode_signed_word_le(bytes[4], bytes[5]);
  calibration.dig_P1 =
      sensors::decoding::decode_unsigned_word_le(bytes[6], bytes[7]);
  calibration.dig_P2 =
      sensors::decoding::decode_signed_word_le(bytes[8], bytes[9]);
  calibration.dig_P3 =
      sensors::decoding::decode_signed_word_le(bytes[10], bytes[11]);
  calibration.dig_P4 =
      sensors::decoding::decode_signed_word_le(bytes[12], bytes[13]);
  calibration.dig_P5 =
      sensors::decoding::decode_signed_word_le(bytes[14], bytes[15]);
  calibration.dig_P6 =
      sensors::decoding::decode_signed_word_le(bytes[16], bytes[17]);
  calibration.dig_P7 =
      sensors::decoding::decode_signed_word_le(bytes[18], bytes[19]);
  calibration.dig_P8 =
      sensors::decoding::decode_signed_word_le(bytes[20], bytes[21]);
  calibration.dig_P9 =
      sensors::decoding::decode_signed_word_le(bytes[22], bytes[23]);
  return true;
}

bool read_measurements_raw(MeasurementsRaw &measurements) {
  std::uint8_t bytes[measurement_size]{};

  const auto result = i2c_bus->read_registers(
      address, measurement_start_register, bytes, measurement_size);

  if (result != drivers::i2c::ReadResult::success) {
    return false;
  }

  measurements.pressure =
      sensors::decoding::decode_unsigned_20bit_be(bytes[0], bytes[1], bytes[2]);

  measurements.temperature =
      sensors::decoding::decode_unsigned_20bit_be(bytes[3], bytes[4], bytes[5]);
  return true;
}

bool read_measurements(Measurements &measurements) {
  if (!calibration_available) {
    return false;
  }

  MeasurementsRaw raw{};

  if (!read_measurements_raw(raw)) {
    return false;
  }

  measurements.temperature_c =
      compensate_temperature(calibration_data, raw.temperature);

  const float pressure_pa =
      compensate_pressure(calibration_data, raw.pressure, raw.temperature);

  measurements.pressure_hpa = pressure_pa / pascals_per_hectopascal;

  return true;
}

void set_i2c_bus(const i2c::I2cBus &bus) { i2c_bus = &bus; }

} // namespace drivers::bmp280
