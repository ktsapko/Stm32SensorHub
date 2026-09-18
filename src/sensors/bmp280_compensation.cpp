#include "drivers/bmp280.hpp"

#include <cstdint>

namespace drivers::bmp280 {
namespace {

constexpr float temperature_adc_scale_1 = 16'384.0F;
constexpr float temperature_adc_scale_2 = 131'072.0F;
constexpr float temperature_calibration_scale_1 = 1'024.0F;
constexpr float temperature_calibration_scale_2 = 8'192.0F;
constexpr float temperature_result_scale = 5'120.0F;

constexpr float pressure_temperature_offset = 64'000.0F;
constexpr float pressure_scale_32k = 32'768.0F;
constexpr float pressure_scale_512k = 524'288.0F;
constexpr float pressure_scale_64k = 65'536.0F;
constexpr float pressure_adc_limit = 1'048'576.0F;
constexpr float pressure_scale_4k = 4'096.0F;
constexpr float pressure_scale_6250 = 6'250.0F;
constexpr float pressure_scale_2g = 2'147'483'648.0F;
constexpr float pressure_final_scale = 16.0F;

float calculate_temperature_fine(const CalibrationData &calibration,
                                 const std::uint32_t raw_temperature) {
  const float adc_temperature = static_cast<float>(raw_temperature);

  const float variable_1 = (adc_temperature / temperature_adc_scale_1 -
                            static_cast<float>(calibration.dig_T1) /
                                temperature_calibration_scale_1) *
                           static_cast<float>(calibration.dig_T2);

  const float difference =
      adc_temperature / temperature_adc_scale_2 -
      static_cast<float>(calibration.dig_T1) / temperature_calibration_scale_2;

  const float variable_2 =
      difference * difference * static_cast<float>(calibration.dig_T3);

  return variable_1 + variable_2;
}

} // namespace

float compensate_temperature(const CalibrationData &calibration,
                             const std::uint32_t raw_temperature) {
  const float temperature_fine =
      calculate_temperature_fine(calibration, raw_temperature);

  return temperature_fine / temperature_result_scale;
}

float compensate_pressure(const CalibrationData &calibration,
                          const std::uint32_t raw_pressure,
                          const std::uint32_t raw_temperature) {
  const float temperature_fine =
      calculate_temperature_fine(calibration, raw_temperature);

  float variable_1 = temperature_fine / 2.0F - pressure_temperature_offset;

  float variable_2 = variable_1 * variable_1 *
                     static_cast<float>(calibration.dig_P6) /
                     pressure_scale_32k;

  variable_2 += variable_1 * static_cast<float>(calibration.dig_P5) * 2.0F;

  variable_2 = variable_2 / 4.0F +
               static_cast<float>(calibration.dig_P4) * pressure_scale_64k;

  variable_1 = (static_cast<float>(calibration.dig_P3) * variable_1 *
                    variable_1 / pressure_scale_512k +
                static_cast<float>(calibration.dig_P2) * variable_1) /
               pressure_scale_512k;

  variable_1 = (1.0F + variable_1 / pressure_scale_32k) *
               static_cast<float>(calibration.dig_P1);

  if (variable_1 == 0.0F) {
    return 0.0F;
  }

  float pressure = pressure_adc_limit - static_cast<float>(raw_pressure);

  pressure = (pressure - variable_2 / pressure_scale_4k) * pressure_scale_6250 /
             variable_1;

  variable_1 = static_cast<float>(calibration.dig_P9) * pressure * pressure /
               pressure_scale_2g;

  variable_2 =
      pressure * static_cast<float>(calibration.dig_P8) / pressure_scale_32k;

  pressure +=
      (variable_1 + variable_2 + static_cast<float>(calibration.dig_P7)) /
      pressure_final_scale;

  return pressure;
}

} // namespace drivers::bmp280
