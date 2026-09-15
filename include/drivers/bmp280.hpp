#pragma once

#include <cstdint>

namespace drivers::bmp280 {

struct CalibrationData {
  std::uint16_t dig_T1;
  std::int16_t dig_T2;
  std::int16_t dig_T3;

  std::uint16_t dig_P1;
  std::int16_t dig_P2;
  std::int16_t dig_P3;
  std::int16_t dig_P4;
  std::int16_t dig_P5;
  std::int16_t dig_P6;
  std::int16_t dig_P7;
  std::int16_t dig_P8;
  std::int16_t dig_P9;
};

struct MeasurementsRaw {
  std::uint32_t pressure;
  std::uint32_t temperature;
};

bool read_chip_id(std::uint8_t &chip_id);
bool initialize();
bool read_calibration(CalibrationData &calibration_data);
bool read_measurements_raw(MeasurementsRaw &measurements);
} // namespace drivers::bmp280