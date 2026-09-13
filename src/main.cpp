#include "drivers/bmp280.hpp"
#include "drivers/i2c1.hpp"
#include "drivers/mpu6050.hpp"
#include "drivers/usart2.hpp"

#include "mcu/gpio.hpp"
#include "mcu/rcc.hpp"

#include <cstdint>

namespace {

constexpr std::uint32_t led_pin = 5U;

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

void write_hex_word(const std::uint16_t value) {
  constexpr char digits[] = "0123456789ABCDEF";

  drivers::usart2::write("0x");
  for (std::uint32_t shift = 16U; shift > 0U; shift -= 4U) {
    drivers::usart2::write_byte(digits[(value >> (shift - 4U)) & 0x0FU]);
  }
}

void report_bmp280() {
  std::uint8_t chip_id = 0U;

  if (drivers::bmp280::read_chip_id(chip_id)) {
    drivers::usart2::write("BMP280 chip ID = ");
    write_hex_byte(chip_id);
    drivers::usart2::write("\r\n");
  } else {
    drivers::usart2::write("BMP280 communication failed\r\n");
  }
}

void report_mpu6050() {
  std::uint8_t identity = 0U;

  if (drivers::mpu6050::read_identity(identity)) {
    drivers::usart2::write("MPU-6050 identity = ");
    write_hex_byte(identity);
    drivers::usart2::write("\r\n");
  } else {
    drivers::usart2::write("MPU-6050 communication failed\r\n");
    return;
  }

  if (!drivers::mpu6050::wake_up()) {
    drivers::usart2::write("MPU-6050 wake-up failed\r\n");
    return;
  }

  if (drivers::mpu6050::is_awake()) {
    drivers::usart2::write("MPU-6050 is awake\r\n");
  } else {
    drivers::usart2::write("MPU-6050 is still sleeping\r\n");
    return;
  }

  drivers::mpu6050::MeasurementsRaw measurements{};
  if (drivers::mpu6050::read_measurements_raw(measurements)) {
    drivers::usart2::write("MPU-6050 acceleration: X=");
    write_hex_word(static_cast<std::uint16_t>(measurements.acceleration.x));
    drivers::usart2::write(" Y=");
    write_hex_word(static_cast<std::uint16_t>(measurements.acceleration.y));
    drivers::usart2::write(" Z=");
    write_hex_word(static_cast<std::uint16_t>(measurements.acceleration.z));
    drivers::usart2::write("\r\n");

    drivers::usart2::write("MPU-6050 temperature raw = ");
    write_hex_word(static_cast<std::uint16_t>(measurements.temperature));
    drivers::usart2::write("\r\n");

    drivers::usart2::write("MPU-6050 angular velocity: X=");
    write_hex_word(static_cast<std::uint16_t>(measurements.angular_velocity.x));
    drivers::usart2::write(" Y=");
    write_hex_word(static_cast<std::uint16_t>(measurements.angular_velocity.y));
    drivers::usart2::write(" Z=");
    write_hex_word(static_cast<std::uint16_t>(measurements.angular_velocity.z));
    drivers::usart2::write("\r\n");
  } else {
    drivers::usart2::write("MPU-6050 measurement read failed\r\n");
  }
}

[[noreturn]] void blink_forever() {
  while (true) {
    set_led(true);
    delay(2'000'000U);

    set_led(false);
    delay(2'000'000U);
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

  report_bmp280();
  report_mpu6050();

  blink_forever();
}