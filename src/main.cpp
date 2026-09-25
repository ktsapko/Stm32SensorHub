#include "diagnostics/i2c_scanner.hpp"
#include "drivers/bmp280.hpp"
#include "drivers/i2c1.hpp"
#include "drivers/mpu6050.hpp"
#include "drivers/oled.hpp"
#include "drivers/systick.hpp"
#include "drivers/usart2.hpp"
#include "graphics/sensor_dashboard.hpp"
#include "graphics/text.hpp"

#include "mcu/gpio.hpp"
#include "mcu/rcc.hpp"

#include <cstdint>

namespace {

constexpr std::uint32_t led_pin = 5U;

constexpr std::uint32_t sensor_startup_delay_ms = 100U;
constexpr std::uint32_t sampling_period_ms = 1'000U;

constexpr bool enable_i2c_scanner = false;

constexpr drivers::i2c::I2cBus i2c_bus{
    .read_register = drivers::i2c1::read_register,
    .read_registers = drivers::i2c1::read_registers,
    .write_register = drivers::i2c1::write_register,
};

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
    drivers::usart2::write("BMP280 communication failed\r\n");
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

  // USART2 output
  drivers::usart2::write("BMP280 temperature = ");
  write_fixed_2(measurements.temperature_c);
  drivers::usart2::write(" C\r\n");

  drivers::usart2::write("BMP280 pressure = ");
  write_fixed_2(measurements.pressure_hpa);
  drivers::usart2::write(" hPa\r\n");

  // OLED output
  if (!graphics::sensor_dashboard::render(measurements)) {
    drivers::usart2::write("OLED dashboard rendering failed\r\n");
    return;
  }

  if (!drivers::oled::flush()) {
    drivers::usart2::write("OLED dashboard flush failed\r\n");
  }
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

  drivers::usart2::write("Calibrating MPU-6050 gyroscope...\r\n");

  if (!drivers::mpu6050::calibrate_gyroscope()) {
    drivers::usart2::write("MPU-6050 gyroscope calibration failed\r\n");
    return false;
  }

  drivers::usart2::write("MPU-6050 gyroscope calibration succeeded\r\n");

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

void report_sensor_sample(const bool bmp280_ready, const bool mpu6050_ready) {
  drivers::usart2::write("\r\n--- Sensor sample ---\r\n");

  if (bmp280_ready) {
    report_bmp280_measurements();
  }

  if (mpu6050_ready) {
    report_mpu6050_measurements();
  }
}

void report_diagnostics() {
  drivers::usart2::write("Dropped TX bytes = ");
  write_unsigned_decimal(drivers::usart2::dropped_tx_bytes());
  drivers::usart2::write("\r\n");
  drivers::usart2::write("Dropped RX bytes = ");
  write_unsigned_decimal(drivers::usart2::dropped_rx_bytes());
  drivers::usart2::write("\r\n");
  drivers::usart2::write("RX overruns = ");
  write_unsigned_decimal(drivers::usart2::hardware_rx_overruns());
  drivers::usart2::write("\r\n");
  drivers::usart2::write("I2C recovery count = ");
  write_unsigned_decimal(drivers::i2c1::recovery_count());
  drivers::usart2::write("\r\n");
}

[[noreturn]] void sample_forever(const bool bmp280_ready,
                                 const bool mpu6050_ready) {
  std::uint32_t last_sample_time =
      drivers::systick::milliseconds() - sampling_period_ms;

  bool led_enabled = false;
  std::uint32_t last_recovery_count = drivers::i2c1::recovery_count();

  bool reporting_enabled = true;

  while (true) {
    const std::uint32_t current_time = drivers::systick::milliseconds();

    if ((current_time - last_sample_time) >= sampling_period_ms) {
      last_sample_time += sampling_period_ms;

      led_enabled = !led_enabled;
      set_led(led_enabled);

      if (reporting_enabled) {
        report_sensor_sample(bmp280_ready, mpu6050_ready);
      }
    }

    const auto current_recovery_count = drivers::i2c1::recovery_count();

    if (current_recovery_count != last_recovery_count) {
      drivers::usart2::write("I2C bus recovery count = ");
      write_unsigned_decimal(current_recovery_count);
      drivers::usart2::write("\r\n");

      last_recovery_count = current_recovery_count;
    }

    std::uint8_t received_byte = 0U;
    constexpr std::uint8_t iteration_limit = 32U;

    for (std::uint8_t i = 0U; i < iteration_limit; ++i) {
      if (!drivers::usart2::read_byte(received_byte)) {
        break;
      }

      if (received_byte == 'p') {
        reporting_enabled = !reporting_enabled;
        if (reporting_enabled) {
          drivers::usart2::write("Reporting resumed\r\n");
        } else {
          drivers::usart2::write("Reporting paused\r\n");
        }
      } else if (received_byte == 's') {
        report_sensor_sample(bmp280_ready, mpu6050_ready);
      } else if (received_byte == 'd') {
        report_diagnostics();
      } else if (received_byte == 'i') {
        diagnostics::i2c_scanner::scan();
      } else {
        drivers::usart2::write_byte(static_cast<char>(received_byte));
      }
    }

    asm volatile("wfi");
  }
}
} // namespace

int main() {
  initialize_led();

  drivers::usart2::initialize();
  drivers::usart2::write("Stm32SensorHub started\r\n");

  drivers::i2c1::initialize();
  drivers::bmp280::set_i2c_bus(i2c_bus);
  drivers::mpu6050::set_i2c_bus(i2c_bus);
  drivers::systick::initialize();

  drivers::systick::delay_ms(sensor_startup_delay_ms);

  if constexpr (enable_i2c_scanner) {
    diagnostics::i2c_scanner::scan();
  }

  drivers::usart2::write("Initializing OLED...\r\n");

  if (drivers::oled::initialize()) {
    drivers::usart2::write("OLED initialized successfully\r\n");

    drivers::oled::clear();

    graphics::draw_text(20U, 20U, "VIKUSIA");
    graphics::draw_text(20U, 32U, "SENSOR HUB");

    if (drivers::oled::flush()) {
      drivers::usart2::write("OLED text rendered successfully\r\n");
    } else {
      drivers::usart2::write("OLED text rendering failed\r\n");
    }
  }

  const bool bmp280_ready = initialize_bmp280();
  const bool mpu6050_ready = initialize_mpu6050();

  sample_forever(bmp280_ready, mpu6050_ready);
}