#include "drivers/mpu6050.hpp"

#include "drivers/i2c1.hpp"

#include <cstdint>

namespace drivers::mpu6050 {
namespace {

constexpr std::uint8_t address = 0x68U;
constexpr std::uint8_t identity_register = 0x75U;
constexpr std::uint8_t power_management_register = 0x6BU;
constexpr std::uint8_t sleep_bit = 1U << 6U;

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
} // namespace drivers::mpu6050