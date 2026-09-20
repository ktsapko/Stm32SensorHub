#pragma once

#include <cstdint>

namespace graphics {

void draw_char(std::uint8_t x, std::uint8_t y, char character);

void draw_text(std::uint8_t x, std::uint8_t y, const char *text);

} // namespace graphics
