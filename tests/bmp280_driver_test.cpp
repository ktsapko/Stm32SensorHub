#include "drivers/bmp280.hpp"
#include "mocks/mock_i2c.hpp"

#include <cstdint>
#include <gtest/gtest.h>

namespace {

constexpr std::uint8_t bmp280_address = 0x76U;
constexpr std::uint8_t chip_id_register = 0xD0U;
constexpr std::uint8_t expected_chip_id = 0x58U;

TEST(Bmp280DriverTest, ReadsChipIdFromI2cBus) {
  tests::mocks::MockI2c mock_i2c;

  mock_i2c.set_register(bmp280_address, chip_id_register, expected_chip_id);

  drivers::bmp280::set_i2c_bus(mock_i2c.bus());

  std::uint8_t chip_id = 0U;

  EXPECT_TRUE(drivers::bmp280::read_chip_id(chip_id));
  EXPECT_EQ(chip_id, expected_chip_id);
}

TEST(Bmp280DriverTest, FailsToReadChipIdWhenI2cReadFails) {
  tests::mocks::MockI2c mock_i2c;

  mock_i2c.set_read_result(drivers::i2c::ReadResult::address_not_acknowledged);

  drivers::bmp280::set_i2c_bus(mock_i2c.bus());

  std::uint8_t chip_id = 0U;

  EXPECT_FALSE(drivers::bmp280::read_chip_id(chip_id));
}

} // namespace
