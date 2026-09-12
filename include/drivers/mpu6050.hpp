#pragma once

#include <cstdint>

namespace drivers::mpu6050 {

bool read_identity(std::uint8_t &identity);
bool wake_up();
bool is_awake();
bool read_acceleration_x_raw(std::int16_t &value);

} // namespace drivers::mpu6050