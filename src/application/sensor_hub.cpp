#include "application/sensor_hub.hpp"

#include "diagnostics/i2c_scanner.hpp"
#include "drivers/i2c1.hpp"
#include "drivers/oled.hpp"
#include "drivers/usart2.hpp"
#include "graphics/sensor_dashboard.hpp"
#include "graphics/text.hpp"

namespace {

constexpr drivers::i2c::I2cBus i2c_bus{
    .read_register = drivers::i2c1::read_register,
    .read_registers = drivers::i2c1::read_registers,
    .write_register = drivers::i2c1::write_register,
};

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

void report_bmp280_measurements(
    const drivers::bmp280::Measurements &measurements) {
  // USART2 output
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

  drivers::usart2::write("Calibrating MPU-6050 gyroscope...\r\n");

  if (!drivers::mpu6050::calibrate_gyroscope()) {
    drivers::usart2::write("MPU-6050 gyroscope calibration failed\r\n");
    return false;
  }

  drivers::usart2::write("MPU-6050 gyroscope calibration succeeded\r\n");

  return true;
}

void report_mpu6050_measurements(
    const drivers::mpu6050::Measurements &measurements) {

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

} // namespace

namespace application::sensor_hub {

Readiness initialize() {
  drivers::i2c1::initialize();
  drivers::bmp280::set_i2c_bus(i2c_bus);
  drivers::mpu6050::set_i2c_bus(i2c_bus);

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
  return {bmp280_ready, mpu6050_ready};
}

Sample acquire_sample(const Readiness &ready) {
  Sample sample{};
  if (ready.bmp280) {
    sample.bmp280_valid = drivers::bmp280::read_measurements(sample.bmp280);
  }
  if (ready.mpu6050) {
    sample.mpu6050_valid = drivers::mpu6050::read_measurements(sample.mpu6050);
  }
  return sample;
}

void report_sample(const Sample &sample) {
  if (sample.bmp280_valid) {
    report_bmp280_measurements(sample.bmp280);
  } else {
    drivers::usart2::write("BMP280 measurement unavailable\r\n");
  }
  if (sample.mpu6050_valid) {
    report_mpu6050_measurements(sample.mpu6050);
  } else {
    drivers::usart2::write("MPU6050 measurements unavailable\r\n");
  }
}

void display_sample(const Sample &sample) {
  if (!graphics::sensor_dashboard::render(sample.bmp280, sample.bmp280_valid,
                                          sample.mpu6050, sample.mpu6050_valid)) {
    drivers::usart2::write("OLED dashboard rendering failed\r\n");
    return;
  }
  if (!drivers::oled::flush()) {
    drivers::usart2::write("OLED dashboard flush failed\r\n");
  }
}

void report_diagnostics() {
  const auto dma_diagnostics = drivers::usart2::dma_tx_diagnostics();
  drivers::usart2::write("DMA TX diagnostics:\r\n");
  drivers::usart2::write("Transfer errors = ");
  write_unsigned_decimal(dma_diagnostics.transfer_errors);
  drivers::usart2::write("\r\n");
  drivers::usart2::write("Direct mode errors = ");
  write_unsigned_decimal(dma_diagnostics.direct_mode_errors);
  drivers::usart2::write("\r\n");
  drivers::usart2::write("FIFO errors = ");
  write_unsigned_decimal(dma_diagnostics.fifo_errors);
  drivers::usart2::write("\r\n");
  drivers::usart2::write("Completed transfers = ");
  write_unsigned_decimal(dma_diagnostics.completed_transfers);
  drivers::usart2::write("\r\n");
  drivers::usart2::write("Recovery attempts = ");
  write_unsigned_decimal(dma_diagnostics.recovery_attempts);
  drivers::usart2::write("\r\n");
  drivers::usart2::write("Uncertain bytes = ");
  write_unsigned_decimal(dma_diagnostics.uncertain_bytes);
  drivers::usart2::write("\r\n");
  drivers::usart2::write("Recovery failures = ");
  write_unsigned_decimal(dma_diagnostics.recovery_failures);
  drivers::usart2::write("\r\n");
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

void handle_command(const std::uint8_t byte, const Readiness &ready,
                    bool &reporting_enabled) {
  if (byte == 'p') {
    reporting_enabled = !reporting_enabled;
    drivers::usart2::write(reporting_enabled ? "Reporting resumed\r\n"
                                            : "Reporting paused\r\n");
  } else if (byte == 's') {
    const auto sample = acquire_sample(ready);
    report_sample(sample);
    display_sample(sample);
  } else if (byte == 'd') {
    report_diagnostics();
  } else if (byte == 'i') {
    diagnostics::i2c_scanner::scan();
  } else {
    drivers::usart2::write_byte(static_cast<char>(byte));
  }
}

} // namespace application::sensor_hub
