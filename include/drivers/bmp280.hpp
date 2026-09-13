#pragma once

#include <cstdint>

namespace drivers::bmp280 {
bool read_chip_id(std::uint8_t &chip_id);
bool initialize();
} // namespace drivers::bmp280