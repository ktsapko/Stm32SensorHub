#pragma once

#include <cstdint>

namespace sensors::decoding {

constexpr std::uint32_t bits_per_byte = 8U;
constexpr std::uint16_t signed_word_threshold = 0x8000U;
constexpr std::int32_t signed_word_modulus = 65'536;

constexpr std::uint16_t decode_unsigned_word_le(const std::uint8_t low,
                                                const std::uint8_t high) {
  return static_cast<std::uint16_t>(
      static_cast<std::uint16_t>(low) |
      (static_cast<std::uint16_t>(high) << bits_per_byte));
}

constexpr std::int16_t decode_signed_word_le(const std::uint8_t low,
                                             const std::uint8_t high) {
  const auto raw_value = decode_unsigned_word_le(low, high);

  const auto signed_value =
      static_cast<std::int32_t>(raw_value) -
      (raw_value >= signed_word_threshold ? signed_word_modulus : 0);

  return static_cast<std::int16_t>(signed_value);
}

constexpr std::int16_t decode_signed_word_be(const std::uint8_t high,
                                             const std::uint8_t low) {
  return decode_signed_word_le(low, high);
}

constexpr std::uint32_t decode_unsigned_20bit_be(const std::uint8_t msb,
                                                 const std::uint8_t lsb,
                                                 const std::uint8_t xlsb) {
  return (static_cast<std::uint32_t>(msb) << 12U) |
         (static_cast<std::uint32_t>(lsb) << 4U) |
         (static_cast<std::uint32_t>(xlsb) >> 4U);
}

} // namespace sensors::decoding
