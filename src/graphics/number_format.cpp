#include "graphics/number_format.hpp"

#include <cstddef>
#include <cstdint>

namespace graphics {

namespace {

constexpr float scale = 100.0F;
constexpr float max_supported_value = 1'000'000.0F;

} // namespace

bool format_fixed_2(const float value, char *const buffer,
                    const std::size_t size) {

  if (buffer == nullptr || size == 0U) {
    return false;
  }

  if (!(value >= -max_supported_value && value <= max_supported_value)) {
    buffer[0] = '\0';
    return false;
  }

  const bool negative = value < 0.0F;

  const float absolute_value = negative ? -value : value;

  const auto scaled = static_cast<std::uint32_t>(absolute_value * scale + 0.5F);

  std::uint32_t integer_part = scaled / 100U;
  const std::uint32_t fractional_part = scaled % 100U;

  char digits[10]{};
  std::size_t digit_count = 0U;

  do {
    digits[digit_count++] = static_cast<char>('0' + integer_part % 10U);

    integer_part /= 10U;

  } while (integer_part != 0U);

  const std::size_t required_size = digit_count + (negative ? 1U : 0U) + 3U;

  if (size < required_size) {
    buffer[0] = '\0';
    return false;
  }

  std::size_t index = 0U;

  if (negative) {
    buffer[index++] = '-';
  }

  while (digit_count != 0U) {
    buffer[index++] = digits[--digit_count];
  }

  buffer[index++] = '.';

  buffer[index++] = static_cast<char>('0' + fractional_part / 10U);

  buffer[index++] = static_cast<char>('0' + fractional_part % 10U);

  buffer[index] = '\0';

  return true;
}

} // namespace graphics
