#include "sensors/decoding.hpp"

#include <gtest/gtest.h>

TEST(DecodingTest, DecodesUnsignedLittleEndianWord) {
  constexpr std::uint8_t low = 0x34U;
  constexpr std::uint8_t high = 0x12U;

  const auto result = sensors::decoding::decode_unsigned_word_le(low, high);

  EXPECT_EQ(result, 0x1234U);
}

TEST(DecodingTest, DecodesSignedPositiveLittleEndianWord) {
  constexpr std::uint8_t low = 0x34U;
  constexpr std::uint8_t high = 0x12U;

  const auto result = sensors::decoding::decode_signed_word_le(low, high);

  EXPECT_EQ(result, 0x1234);
}

TEST(DecodingTest, DecodesSignedNegativeLittleEndianWord) {
  constexpr std::uint8_t low = 0x00U;
  constexpr std::uint8_t high = 0x80U;

  const auto result = sensors::decoding::decode_signed_word_le(low, high);

  EXPECT_EQ(result, -32768);
}

TEST(DecodingTest, DecodesSignedBigEndianWord) {
  constexpr std::uint8_t high = 0xFFU;
  constexpr std::uint8_t low = 0xFFU;

  const auto result = sensors::decoding::decode_signed_word_be(high, low);

  EXPECT_EQ(result, -1);
}

TEST(DecodingTest, DecodesUnsigned20BitBigEndianValue) {
  constexpr std::uint8_t msb = 0x12U;
  constexpr std::uint8_t lsb = 0x34U;
  constexpr std::uint8_t xlsb = 0x50U;

  const auto result =
      sensors::decoding::decode_unsigned_20bit_be(msb, lsb, xlsb);

  EXPECT_EQ(result, 0x12345U);
}
