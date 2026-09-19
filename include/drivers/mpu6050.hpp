#pragma once

#include "drivers/i2c.hpp"

#include <cstdint>

namespace drivers::mpu6050 {
struct Acceleration {
  std::int16_t x;
  std::int16_t y;
  std::int16_t z;
};

struct AngularVelocityRaw {
  std::int16_t x;
  std::int16_t y;
  std::int16_t z;
};

struct MeasurementsRaw {
  Acceleration acceleration;
  std::int16_t temperature;
  AngularVelocityRaw angular_velocity;
};

struct Vector3 {
  float x;
  float y;
  float z;
};

struct Measurements {
  Vector3 acceleration_g;
  float temperature_c;
  Vector3 angular_velocity_dps;
};

struct GyroscopeBias {
  float x_deg_per_s;
  float y_deg_per_s;
  float z_deg_per_s;
};

bool read_identity(std::uint8_t &identity);
bool wake_up();
bool is_awake();
bool read_acceleration_x_raw(std::int16_t &value);
bool read_acceleration_raw(Acceleration &acceleration);
bool read_measurements_raw(MeasurementsRaw &measurements);
bool read_measurements(Measurements &measurements);
bool calibrate_gyroscope();
void set_i2c_bus(const i2c::I2cBus &bus);

} // namespace drivers::mpu6050
