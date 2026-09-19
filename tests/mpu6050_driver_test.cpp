#include "drivers/mpu6050.hpp"
#include "mocks/mock_i2c.hpp"

#include <cstdint>
#include <gtest/gtest.h>

namespace {

constexpr std::uint8_t mpu6050_address = 0x68U;
constexpr std::uint8_t who_am_i_register = 0x75U;
constexpr std::uint8_t expected_identity = 0x68U;

constexpr std::uint8_t power_management_register = 0x6BU;
constexpr std::uint8_t wake_value = 0x00U;

TEST(Mpu6050DriverTest, ReadsIdentityFromI2cBus) {
  tests::mocks::MockI2c mock_i2c;

  mock_i2c.set_register(mpu6050_address, who_am_i_register, expected_identity);

  drivers::mpu6050::set_i2c_bus(mock_i2c.bus());

  std::uint8_t identity = 0U;

  EXPECT_TRUE(drivers::mpu6050::read_identity(identity));
  EXPECT_EQ(identity, expected_identity);
}

TEST(Mpu6050DriverTest, FailsToReadIdentityWhenI2cReadFails) {
  tests::mocks::MockI2c mock_i2c;

  mock_i2c.set_read_result(drivers::i2c::ReadResult::address_not_acknowledged);

  drivers::mpu6050::set_i2c_bus(mock_i2c.bus());

  std::uint8_t identity = 0U;

  EXPECT_FALSE(drivers::mpu6050::read_identity(identity));
}

TEST(Mpu6050DriverTest, WakesUpDevice) {
  tests::mocks::MockI2c mock_i2c;

  mock_i2c.set_register(mpu6050_address, power_management_register, 0x40U);

  drivers::mpu6050::set_i2c_bus(mock_i2c.bus());

  EXPECT_TRUE(drivers::mpu6050::wake_up());

  EXPECT_EQ(mock_i2c.get_register(mpu6050_address, power_management_register),
            wake_value);
}

} // namespace
