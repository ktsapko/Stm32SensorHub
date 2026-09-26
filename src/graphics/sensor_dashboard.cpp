#include "graphics/sensor_dashboard.hpp"

#include "drivers/oled.hpp"
#include "graphics/font5x7.hpp"
#include "graphics/number_format.hpp"
#include "graphics/text.hpp"

namespace graphics::sensor_dashboard {
namespace {

bool draw_value(const std::uint8_t x, const std::uint8_t y,
                const float value) {
  // Each value column accommodates at most seven characters.
  constexpr std::uint8_t field_characters = 7U;
  char text[field_characters + 1U]{};
  if (!format_fixed_2(value, text, sizeof(text))) {
    return false;
  }
  std::uint8_t length = 0U;
  while (text[length] != '\0') {
    ++length;
  }
  const auto offset = (field_characters - length) *
                      (font5x7::width + font5x7::spacing);
  draw_text(static_cast<std::uint8_t>(x + offset), y, text);
  return true;
}

} // namespace

bool render(const drivers::bmp280::Measurements &bmp280_measurements,
            const bool bmp280_valid,
            const drivers::mpu6050::Measurements &mpu6050_measurements,
            const bool mpu6050_valid) {
  drivers::oled::clear();

  draw_text(0U, 0U, "BMP T:");
  draw_text(0U, 8U, "BMP P:");
  if (bmp280_valid) {
    if (!draw_value(42U, 0U, bmp280_measurements.temperature_c) ||
        !draw_value(42U, 8U, bmp280_measurements.pressure_hpa)) {
      return false;
    }
    draw_text(90U, 0U, "C");
    draw_text(90U, 8U, "hPa");
  } else {
    draw_text(42U, 0U, "ERR");
    draw_text(42U, 8U, "ERR");
  }

  draw_text(0U, 16U, "MPU T:");
  draw_text(18U, 32U, "A g");
  draw_text(78U, 32U, "W deg/s");

  const char *const axes[] = {"X", "Y", "Z"};
  for (std::uint8_t axis = 0U; axis < 3U; ++axis) {
    draw_text(0U, static_cast<std::uint8_t>(40U + axis * 8U), axes[axis]);
  }

  if (!mpu6050_valid) {
    draw_text(42U, 16U, "ERR");
    for (std::uint8_t axis = 0U; axis < 3U; ++axis) {
      const auto y = static_cast<std::uint8_t>(40U + axis * 8U);
      draw_text(18U, y, "ERR");
      draw_text(78U, y, "ERR");
    }
    return true;
  }

  if (!draw_value(42U, 16U, mpu6050_measurements.temperature_c)) {
    return false;
  }
  draw_text(90U, 16U, "C");

  const auto &a = mpu6050_measurements.acceleration_g;
  const auto &w = mpu6050_measurements.angular_velocity_dps;
  return draw_value(18U, 40U, a.x) && draw_value(78U, 40U, w.x) &&
         draw_value(18U, 48U, a.y) && draw_value(78U, 48U, w.y) &&
         draw_value(18U, 56U, a.z) && draw_value(78U, 56U, w.z);
}

} // namespace graphics::sensor_dashboard
