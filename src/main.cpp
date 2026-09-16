#include "drivers/bmp280.hpp"
#include "drivers/i2c1.hpp"
#include "drivers/mpu6050.hpp"
#include "drivers/usart2.hpp"

#include "mcu/gpio.hpp"
#include "mcu/rcc.hpp"

#include <cstdint>

namespace {

constexpr std::uint32_t led_pin = 5U;
constexpr std::uint32_t sampling_delay_cycles = 8'000'000U;

void delay(const std::uint32_t cycles) {
  for (std::uint32_t i = 0U; i < cycles; ++i) {
    asm volatile("nop");
  }
}

void initialize_led() {
  mcu::rcc::enable_ahb1(mcu::rcc::ahb1::gpioa);
  mcu::gpio::set_mode(mcu::gpio::gpioa, led_pin, mcu::gpio::Mode::output);
}

void set_led(const bool enabled) {
  mcu::gpio::set_output(mcu::gpio::gpioa, led_pin, enabled);
}

void write_hex_byte(const std::uint8_t value) {
  constexpr char digits[] = "0123456789ABCDEF";

  drivers::usart2::write("0x");
  drivers::usart2::write_byte(digits[(value >> 4U) & 0x0FU]);
  drivers::usart2::write_byte(digits[value & 0x0FU]);
}

void write_unsigned_decimal(std::uint32_t value) {
  char buffer[10]{};
  std::uint32_t length = 0U;

  do {
    buffer[length] = static_cast<char>('0' + value % 10U);
    value /= 10U;
    ++length;
  } while (value != 0U);

  while (length != 0U) {
    --length;
    drivers::usart2::write_byte(buffer[length]);
  }
}

void write_fixed_2(const float value) {
  const auto scaled = static_cast<std::int32_t>(value * 100.0F +
                                                (value >= 0.0F ? 0.5F : -0.5F));

  std::uint32_t magnitude = 0U;

  if (scaled < 0) {
    drivers::usart2::write_byte('-');
    magnitude = static_cast<std::uint32_t>(-scaled);
  } else {
    magnitude = static_cast<std::uint32_t>(scaled);
  }

  write_unsigned_decimal(magnitude / 100U);
  drivers::usart2::write_byte('.');
  drivers::usart2::write_byte(static_cast<char>('0' + (magnitude / 10U) % 10U));
  drivers::usart2::write_byte(static_cast<char>('0' + magnitude % 10U));
}

bool initialize_bmp280() {
  std::uint8_t chip_id = 0U;

  if (!drivers::bmp280::read_chip_id(chip_id)) {
    drivers::usart2::write("BMP280 communication flied\r\n");
    return false;
  }

  drivers::usart2::write("BMP280 chip ID = ");
  write_hex_byte(chip_id);
  drivers::usart2::write("\r\n");

  if (!drivers::bmp280::initialize()) {
    drivers::usart2::write("BMP280 initialization failed\r\n");
    return false;
  }

  drivers::usart2::write("BMP280 initialized successfully\r\n");
  return true;
}

void report_bmp280_measurements() {
  drivers::bmp280::Measurements measurements{};

  if (!drivers::bmp280::read_measurements(measurements)) {
    drivers::usart2::write("BMP280 measurement read failed\r\n");
    return;
  }

  drivers::usart2::write("BMP280 temperature = ");
  write_fixed_2(measurements.temperature_c);
  drivers::usart2::write(" C\r\n");

  drivers::usart2::write("BMP280 pressure = ");
  write_fixed_2(measurements.pressure_hpa);
  drivers::usart2::write(" hPa\r\n");
}

bool initialize_mpu6050() {
  std::uint8_t identity = 0U;

  if (!drivers::mpu6050::read_identity(identity)) {
    drivers::usart2::write("MPU-6050 communication failed\r\n");
    return false;
  }

  drivers::usart2::write("MPU-6050 identity = ");
  write_hex_byte(identity);
  drivers::usart2::write("\r\n");

  if (!drivers::mpu6050::wake_up()) {
    drivers::usart2::write("MPU-6050 wake-up failed\r\n");
    return false;
  }

  if (!drivers::mpu6050::is_awake()) {
    drivers::usart2::write("MPU-6050 is still sleeping\r\n");
    return false;
  }

  drivers::usart2::write("MPU-6050 is awake\r\n");
  return true;
}

void report_mpu6050_measurements() {
  drivers::mpu6050::Measurements measurements{};

  if (!drivers::mpu6050::read_measurements(measurements)) {
    drivers::usart2::write("MPU-6050 measurement read failed\r\n");
    return;
  }

  drivers::usart2::write("Acceleration: X=");
  write_fixed_2(measurements.acceleration_g.x);
  drivers::usart2::write(" g, Y=");
  write_fixed_2(measurements.acceleration_g.y);
  drivers::usart2::write(" g, Z=");
  write_fixed_2(measurements.acceleration_g.z);
  drivers::usart2::write(" g\r\n");

  drivers::usart2::write("Temperature: ");
  write_fixed_2(measurements.temperature_c);
  drivers::usart2::write(" C\r\n");

  drivers::usart2::write("Angular velocity: X=");
  write_fixed_2(measurements.angular_velocity_dps.x);
  drivers::usart2::write(" deg/s, Y=");
  write_fixed_2(measurements.angular_velocity_dps.y);
  drivers::usart2::write(" deg/s, Z=");
  write_fixed_2(measurements.angular_velocity_dps.z);
  drivers::usart2::write(" deg/s\r\n");
}

[[noreturn]] void sample_forever(const bool bmp280_ready,
                                 const bool mpu6050_ready) {
  bool led_enabled = false;

  while (true) {
    led_enabled = !led_enabled;
    set_led(led_enabled);

    drivers::usart2::write("\r\n--- Sensor sample ---\r\n");

    if (bmp280_ready) {
      report_bmp280_measurements();
    }

    if (mpu6050_ready) {
      report_mpu6050_measurements();
    }

    delay(sampling_delay_cycles);
  }
}

} // namespace

int main() {
  initialize_led();

  drivers::usart2::initialize();
  drivers::usart2::write("Stm32SensorHub started\r\n");

  drivers::i2c1::initialize();

  // Give the sensors time to start after power-on.
  delay(2'000'000U);

  const bool bmp280_ready = initialize_bmp280();
  const bool mpu6050_ready = initialize_mpu6050();

  sample_forever(bmp280_ready, mpu6050_ready);
}