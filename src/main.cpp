#include "drivers/i2c1.hpp"
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

[[noreturn]] void blink_forever() {
  while (true) {
    set_led(true);
    delay(2'000'000U);

    set_led(false);
    delay(2'000'000U);
  }
}

void probe_sensor(const char *sensor_name, const std::uint8_t address) {
  drivers::usart2::write("Probing ");
  drivers::usart2::write(sensor_name);
  drivers::usart2::write("...\r\n");

  const auto result = drivers::i2c1::probe(address);

  if (result == drivers::i2c1::ProbeResult::acknowledged) {
    drivers::usart2::write(sensor_name);
    drivers::usart2::write(" found\r\n");
  } else {
    drivers::usart2::write(sensor_name);
    drivers::usart2::write(" not found\r\n");
  }
}

} // namespace

int main() {
  constexpr std::uint8_t bmp280_address = 0x76U;
  constexpr std::uint8_t mpu6050_address = 0x68U;

  initialize_led();

  drivers::usart2::initialize();
  drivers::usart2::write("Stm32SensorHub started\r\n");

  drivers::i2c1::initialize();

  // Give the sensors time to start after power-on.
  delay(2'000'000U);

  probe_sensor("BMP280", bmp280_address);
  probe_sensor("MPU-6050", mpu6050_address);

  blink_forever();
}