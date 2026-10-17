#pragma once

#include "drivers/bmp280.hpp"
#include "drivers/mpu6050.hpp"

#include <cstdint>

namespace application::sensor_hub {

struct Readiness {
  bool bmp280;
  bool mpu6050;
};

struct Sample {
  drivers::bmp280::Measurements bmp280{};
  drivers::mpu6050::Measurements mpu6050{};
  bool bmp280_valid = false;
  bool mpu6050_valid = false;
};

// Caller must initialize USART2 and allow device power-on time before this call.
// Initializes I2C1, OLED and sensors; includes blocking gyroscope calibration.
Readiness initialize();
Sample acquire_sample(const Readiness &ready);
void report_sample(const Sample &sample);
void display_sample(const Sample &sample);
void report_diagnostics();

// Updates caller-owned reporting state; 's' performs one acquisition/report/display.
// These operations are synchronous and require serialized access to I2C/OLED/UART.
void handle_command(std::uint8_t byte, const Readiness &ready,
                    bool &reporting_enabled);

} // namespace application::sensor_hub
