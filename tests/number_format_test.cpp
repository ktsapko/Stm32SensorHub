#include "graphics/number_format.hpp"

#include <gtest/gtest.h>

#include <limits>
#include <string>

namespace {

std::string format(const float value) {
  char buffer[16]{};

  if (!graphics::format_fixed_2(value, buffer, sizeof(buffer))) {
    return {};
  }

  return buffer;
}

} // namespace

TEST(NumberFormatTest, FormatsPositiveTemperature) {
  EXPECT_EQ(format(25.89F), "25.89");
}

TEST(NumberFormatTest, FormatsPressure) {
  EXPECT_EQ(format(1006.56F), "1006.56");
}

TEST(NumberFormatTest, FormatsNegativeValue) {
  EXPECT_EQ(format(-0.14F), "-0.14");
}

TEST(NumberFormatTest, FormatsZero) { EXPECT_EQ(format(0.0F), "0.00"); }

TEST(NumberFormatTest, RoundsToTwoDecimalPlaces) {
  EXPECT_EQ(format(9.999F), "10.00");
}

TEST(NumberFormatTest, FormatsSmallValue) { EXPECT_EQ(format(0.01F), "0.01"); }

TEST(NumberFormatTest, RejectsNullBuffer) {
  EXPECT_FALSE(graphics::format_fixed_2(25.89F, nullptr, 16U));
}

TEST(NumberFormatTest, RejectsEmptyBuffer) {
  char buffer[16]{};

  EXPECT_FALSE(graphics::format_fixed_2(25.89F, buffer, 0U));
}

TEST(NumberFormatTest, RejectsInsufficientBuffer) {
  char buffer[4]{};

  EXPECT_FALSE(graphics::format_fixed_2(25.89F, buffer, sizeof(buffer)));

  EXPECT_EQ(buffer[0], '\0');
}

TEST(NumberFormatTest, AcceptsExactBufferSize) {
  char buffer[6]{};

  EXPECT_TRUE(graphics::format_fixed_2(25.89F, buffer, sizeof(buffer)));

  EXPECT_STREQ(buffer, "25.89");
}

TEST(NumberFormatTest, RejectsNaN) {
  EXPECT_TRUE(format(std::numeric_limits<float>::quiet_NaN()).empty());
}

TEST(NumberFormatTest, RejectsInfinity) {
  EXPECT_TRUE(format(std::numeric_limits<float>::infinity()).empty());
}

TEST(NumberFormatTest, RejectsOutOfRangeValue) {
  EXPECT_TRUE(format(1'000'001.0F).empty());
}