#pragma once

#include "drivers/bmp280.hpp"
#include "drivers/mpu6050.hpp"

namespace graphics::sensor_dashboard {

bool render(const drivers::bmp280::Measurements &bmp280_measurements,
            bool bmp280_valid,
            const drivers::mpu6050::Measurements &mpu6050_measurements,
            bool mpu6050_valid);

} // namespace graphics::sensor_dashboard
