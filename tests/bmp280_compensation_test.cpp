#include "drivers/bmp280.hpp"

#include <gtest/gtest.h>

namespace {

constexpr drivers::bmp280::CalibrationData reference_calibration{
    .dig_T1 = 27504,
    .dig_T2 = 26435,
    .dig_T3 = -1000,
    .dig_P1 = 36477,
    .dig_P2 = -10685,
    .dig_P3 = 3024,
    .dig_P4 = 2855,
    .dig_P5 = 140,
    .dig_P6 = -7,
    .dig_P7 = 15500,
    .dig_P8 = -14600,
    .dig_P9 = 6000,
};

constexpr std::uint32_t reference_raw_temperature = 519888U;
constexpr std::uint32_t reference_raw_pressure = 415148U;

} // namespace

TEST(Bmp280CompensationTest, CompensatesReferenceTemperature) {
  const auto temperature =
      drivers::bmp280::compensate_temperature(reference_calibration,
                                               reference_raw_temperature);

  EXPECT_NEAR(temperature, 25.08F, 0.01F);
}

TEST(Bmp280CompensationTest, CompensatesReferencePressure) {
  const auto pressure =
      drivers::bmp280::compensate_pressure(reference_calibration,
                                           reference_raw_pressure,
                                           reference_raw_temperature);

  EXPECT_NEAR(pressure, 100653.0F, 1.0F);
}
