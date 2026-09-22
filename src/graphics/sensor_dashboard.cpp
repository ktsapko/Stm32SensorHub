#include "graphics/sensor_dashboard.hpp"

#include "drivers/oled.hpp"
#include "graphics/number_format.hpp"
#include "graphics/text.hpp"

namespace graphics::sensor_dashboard {

bool render(const drivers::bmp280::Measurements &measurements) {
  char temperature[16]{};
  char pressure[16]{};

  if (!format_fixed_2(measurements.temperature_c, temperature,
                      sizeof(temperature))) {
    return false;
  }

  if (!format_fixed_2(measurements.pressure_hpa, pressure, sizeof(pressure))) {
    return false;
  }

  drivers::oled::clear();

  // Header
  draw_text(0U, 0U, "STM32 SENSOR HUB");

  // Temperature
  draw_text(0U, 20U, "TEMP:");
  draw_text(42U, 20U, temperature);
  draw_text(84U, 20U, "C");

  // Atmospheric pressure
  draw_text(0U, 32U, "PRESS:");
  draw_text(42U, 32U, pressure);
  draw_text(90U, 32U, "hPa");

  // Sensor identification
  draw_text(0U, 52U, "BMP280");

  return true;
}

} // namespace graphics::sensor_dashboard
