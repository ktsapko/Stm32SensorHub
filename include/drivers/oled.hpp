#pragma once

#include <cstdint>

namespace drivers::oled {

bool initialize();

bool display_all_on();
bool display_normal();

void clear();

void set_pixel(std::uint8_t x, std::uint8_t y);

bool flush();

} // namespace drivers::oled
