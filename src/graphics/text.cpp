#include "graphics/text.hpp"

#include "graphics/font5x7.hpp"

#include "drivers/oled.hpp"

#include <cstdint>

namespace graphics {

namespace {

const font5x7::Glyph *find_glyph(const char character) {
  for (const auto &glyph : font5x7::glyphs) {
    if (glyph.character == character) {
      return &glyph;
    }
  }

  return nullptr;
}

} // namespace

void draw_char(const std::uint8_t x, const std::uint8_t y,
               const char character) {

  const auto *glyph = find_glyph(character);

  if (glyph == nullptr) {
    return;
  }

  for (std::uint8_t column = 0U; column < font5x7::width; ++column) {

    const auto bits = glyph->columns[column];

    for (std::uint8_t row = 0U; row < font5x7::height; ++row) {

      if ((bits & (1U << row)) != 0U) {
        drivers::oled::set_pixel(static_cast<std::uint8_t>(x + column),
                                 static_cast<std::uint8_t>(y + row));
      }
    }
  }
}

void draw_text(const std::uint8_t x, const std::uint8_t y,
               const char *const text) {

  if (text == nullptr) {
    return;
  }

  constexpr std::uint16_t display_width = 128U;

  std::uint16_t cursor_x = x;

  for (const char *current = text; *current != '\0'; ++current) {

    if (cursor_x + font5x7::width > display_width) {
      break;
    }

    draw_char(static_cast<std::uint8_t>(cursor_x), y, *current);

    cursor_x += font5x7::width + font5x7::spacing;
  }
}

} // namespace graphics
