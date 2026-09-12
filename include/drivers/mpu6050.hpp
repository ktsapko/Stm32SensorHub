#pragma once

#include <cstdint>

namespace drivers::mpu6050 {
struct Acceleration {
  std::int16_t x;
  std::int16_t y;
  std::int16_t z;
};

bool read_identity(std::uint8_t &identity);
bool wake_up();
bool is_awake();
bool read_acceleration_x_raw(std::int16_t &value);
bool read_acceleration_raw(Acceleration &acceleration);

} // namespace drivers::mpu6050