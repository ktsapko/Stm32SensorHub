#include "drivers/mpu6050.hpp"

#include "sensors/decoding.hpp"

#include <cstddef>
#include <cstdint>

namespace drivers::mpu6050 {
namespace {

constexpr std::uint8_t address = 0x68U;
constexpr std::uint8_t who_am_i_register = 0x75U;
constexpr std::uint8_t pwr_mgmt_1_register = 0x6BU;
constexpr std::uint8_t sleep_bit = 1U << 6U;
constexpr std::uint8_t accel_xout_h_register = 0x3BU;

// Decode raw measurements to pheysical values. The MPU-6050 datasheet specifies
// the following conversion factors:
constexpr float acceleration_sensitivity = 16384.0f;   // LSB/g for ±2g range
constexpr float angular_velocity_sensitivity = 131.0f; // LSB/(°/s)
constexpr float temperature_sensitivity = 340.0f;      // LSB/°C
constexpr float temperature_offset = 36.53f;           // °C

constexpr std::size_t gyroscope_calibration_sample_count = 100U;

const i2c::I2cBus *i2c_bus = nullptr;

GyroscopeBias gyroscope_bias{
    .x_deg_per_s = 0.0F,
    .y_deg_per_s = 0.0F,
    .z_deg_per_s = 0.0F,
};

bool read_uncalibrated_measurements(Measurements &measurements) {
  MeasurementsRaw raw_measurements{};
  if (!read_measurements_raw(raw_measurements)) {
    return false;
  }

  measurements.acceleration_g.x =
      static_cast<float>(raw_measurements.acceleration.x) /
      acceleration_sensitivity;
  measurements.acceleration_g.y =
      static_cast<float>(raw_measurements.acceleration.y) /
      acceleration_sensitivity;
  measurements.acceleration_g.z =
      static_cast<float>(raw_measurements.acceleration.z) /
      acceleration_sensitivity;

  measurements.temperature_c =
      (static_cast<float>(raw_measurements.temperature) /
       temperature_sensitivity) +
      temperature_offset;

  measurements.angular_velocity_dps.x =
      static_cast<float>(raw_measurements.angular_velocity.x) /
      angular_velocity_sensitivity;
  measurements.angular_velocity_dps.y =
      static_cast<float>(raw_measurements.angular_velocity.y) /
      angular_velocity_sensitivity;
  measurements.angular_velocity_dps.z =
      static_cast<float>(raw_measurements.angular_velocity.z) /
      angular_velocity_sensitivity;

  return true;
}

} // namespace

bool read_identity(std::uint8_t &identity) {
  return i2c_bus->read_register(address, who_am_i_register, identity) ==
         drivers::i2c::ReadResult::success;
}

bool wake_up() {
  return i2c_bus->write_register(address, pwr_mgmt_1_register, 0x00U) ==
         drivers::i2c::WriteResult::success;
}

bool is_awake() {
  std::uint8_t power_management = 0U;
  const auto result =
      i2c_bus->read_register(address, pwr_mgmt_1_register, power_management);

  if (result != drivers::i2c::ReadResult::success) {
    return false;
  }
  return (power_management & sleep_bit) == 0U;
}

bool read_acceleration_x_raw(std::int16_t &value) {
  std::uint8_t bytes[2]{};
  const auto result =
      i2c_bus->read_registers(address, accel_xout_h_register, bytes, 2U);

  if (result != drivers::i2c::ReadResult::success) {
    return false;
  }

  value = sensors::decoding::decode_signed_word_be(bytes[0], bytes[1]);
  return true;
}

bool read_acceleration_raw(Acceleration &acceleration) {
  std::uint8_t bytes[6]{};
  const auto result =
      i2c_bus->read_registers(address, accel_xout_h_register, bytes, 6U);
  if (result != drivers::i2c::ReadResult::success) {
    return false;
  }
  acceleration.x = sensors::decoding::decode_signed_word_be(bytes[0], bytes[1]);
  acceleration.y = sensors::decoding::decode_signed_word_be(bytes[2], bytes[3]);
  acceleration.z = sensors::decoding::decode_signed_word_be(bytes[4], bytes[5]);
  return true;
}

bool read_measurements_raw(MeasurementsRaw &measurements) {
  std::uint8_t bytes[14]{};
  const auto result =
      i2c_bus->read_registers(address, accel_xout_h_register, bytes, 14U);
  if (result != drivers::i2c::ReadResult::success) {
    return false;
  }
  measurements.acceleration.x =
      sensors::decoding::decode_signed_word_be(bytes[0], bytes[1]);
  measurements.acceleration.y =
      sensors::decoding::decode_signed_word_be(bytes[2], bytes[3]);
  measurements.acceleration.z =
      sensors::decoding::decode_signed_word_be(bytes[4], bytes[5]);
  measurements.temperature =
      sensors::decoding::decode_signed_word_be(bytes[6], bytes[7]);
  measurements.angular_velocity.x =
      sensors::decoding::decode_signed_word_be(bytes[8], bytes[9]);
  measurements.angular_velocity.y =
      sensors::decoding::decode_signed_word_be(bytes[10], bytes[11]);
  measurements.angular_velocity.z =
      sensors::decoding::decode_signed_word_be(bytes[12], bytes[13]);
  return true;
}

bool read_measurements(Measurements &measurements) {
  if (!read_uncalibrated_measurements(measurements)) {
    return false;
  }

  measurements.angular_velocity_dps.x -= gyroscope_bias.x_deg_per_s;
  measurements.angular_velocity_dps.y -= gyroscope_bias.y_deg_per_s;
  measurements.angular_velocity_dps.z -= gyroscope_bias.z_deg_per_s;

  return true;
}

bool calibrate_gyroscope() {
  float sum_x = 0.0F;
  float sum_y = 0.0F;
  float sum_z = 0.0F;

  for (std::size_t i = 0U; i < gyroscope_calibration_sample_count; ++i) {
    Measurements measurements{};

    if (!read_uncalibrated_measurements(measurements)) {
      return false;
    }

    sum_x += measurements.angular_velocity_dps.x;
    sum_y += measurements.angular_velocity_dps.y;
    sum_z += measurements.angular_velocity_dps.z;
  }

  const auto sample_count =
      static_cast<float>(gyroscope_calibration_sample_count);

  gyroscope_bias.x_deg_per_s = sum_x / sample_count;
  gyroscope_bias.y_deg_per_s = sum_y / sample_count;
  gyroscope_bias.z_deg_per_s = sum_z / sample_count;
  return true;
}

void set_i2c_bus(const i2c::I2cBus &bus) { i2c_bus = &bus; }

} // namespace drivers::mpu6050
