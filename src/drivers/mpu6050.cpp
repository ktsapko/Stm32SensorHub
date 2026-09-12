#include "drivers/mpu6050.hpp"

#include "drivers/i2c1.hpp"

#include <cstdint>

namespace drivers::mpu6050 {
namespace {

constexpr std::uint8_t address = 0x68U;
constexpr std::uint8_t identity_register = 0x75U;
constexpr std::uint8_t power_management_register = 0x6BU;
constexpr std::uint8_t sleep_bit = 1U << 6U;
constexpr std::uint8_t acceleration_x_high_register = 0x3BU;

std::int16_t decode_signed_word(const std::uint8_t high,
                                const std::uint8_t low) {
  const std::uint16_t raw_value = (static_cast<std::uint16_t>(high) << 8U) |
                                  static_cast<std::uint16_t>(low);

  const std::int32_t signed_value = static_cast<std::int32_t>(raw_value) -
                                    (raw_value >= 0x8000U ? 65'536 : 0);

  return static_cast<std::int16_t>(signed_value);
}

} // namespace

bool read_identity(std::uint8_t &identity) {
  return drivers::i2c1::read_register(address, identity_register, identity) ==
         drivers::i2c1::ReadResult::success;
}

bool wake_up() {
  return drivers::i2c1::write_register(address, power_management_register,
                                       0x00U) ==
         drivers::i2c1::WriteResult::success;
}

bool is_awake() {
  std::uint8_t power_management = 0U;
  const auto result = drivers::i2c1::read_register(
      address, power_management_register, power_management);

  if (result != drivers::i2c1::ReadResult::success) {
    return false;
  }
  return (power_management & sleep_bit) == 0U;
}

bool read_acceleration_x_raw(std::int16_t &value) {
  std::uint8_t bytes[2]{};
  const auto result = drivers::i2c1::read_registers(
      address, acceleration_x_high_register, bytes, 2U);
  if (result != drivers::i2c1::ReadResult::success) {
    return false;
  }
  const std::uint16_t raw_value = (static_cast<std::uint16_t>(bytes[0]) << 8U) |
                                  static_cast<std::uint16_t>(bytes[1]);

  const std::int32_t signed_value = static_cast<std::int32_t>(raw_value) -
                                    (raw_value >= 0x8000U ? 65'536 : 0);

  value = static_cast<std::int16_t>(signed_value);
  return true;
}

bool read_acceleration_raw(Acceleration &acceleration) {
  std::uint8_t bytes[6]{};
  const auto result = drivers::i2c1::read_registers(
      address, acceleration_x_high_register, bytes, 6U);
  if (result != drivers::i2c1::ReadResult::success) {
    return false;
  }

  acceleration.x = decode_signed_word(bytes[0], bytes[1]);
  acceleration.y = decode_signed_word(bytes[2], bytes[3]);
  acceleration.z = decode_signed_word(bytes[4], bytes[5]);

  return true;
}

} // namespace drivers::mpu6050