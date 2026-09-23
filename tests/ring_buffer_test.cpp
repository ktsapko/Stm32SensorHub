
#include "utils/ring_buffer.hpp"

#include <gtest/gtest.h>

#include <cstdint>

namespace {

using utils::RingBuffer;

TEST(RingBufferTest, InitiallyEmpty) {
  RingBuffer<4> buffer;

  EXPECT_TRUE(buffer.empty());
  EXPECT_FALSE(buffer.full());
  EXPECT_EQ(buffer.size(), 0U);
  EXPECT_EQ(buffer.capacity(), 4U);
}

TEST(RingBufferTest, PreservesFifoOrder) {
  RingBuffer<4> buffer;

  EXPECT_TRUE(buffer.push(10U));
  EXPECT_TRUE(buffer.push(20U));
  EXPECT_TRUE(buffer.push(30U));

  std::uint8_t value = 0U;

  ASSERT_TRUE(buffer.pop(value));
  EXPECT_EQ(value, 10U);

  ASSERT_TRUE(buffer.pop(value));
  EXPECT_EQ(value, 20U);

  ASSERT_TRUE(buffer.pop(value));
  EXPECT_EQ(value, 30U);

  EXPECT_TRUE(buffer.empty());
}

TEST(RingBufferTest, RejectsOverflow) {
  RingBuffer<2> buffer;

  EXPECT_TRUE(buffer.push(10U));
  EXPECT_TRUE(buffer.push(20U));

  EXPECT_TRUE(buffer.full());
  EXPECT_FALSE(buffer.push(30U));

  EXPECT_EQ(buffer.size(), 2U);

  std::uint8_t value = 0U;

  ASSERT_TRUE(buffer.pop(value));
  EXPECT_EQ(value, 10U);

  ASSERT_TRUE(buffer.pop(value));
  EXPECT_EQ(value, 20U);
}

TEST(RingBufferTest, RejectsPopWhenEmpty) {
  RingBuffer<2> buffer;

  std::uint8_t value = 42U;

  EXPECT_FALSE(buffer.pop(value));
  EXPECT_EQ(value, 42U);
}

TEST(RingBufferTest, WrapsAround) {
  RingBuffer<3> buffer;

  EXPECT_TRUE(buffer.push(10U));
  EXPECT_TRUE(buffer.push(20U));
  EXPECT_TRUE(buffer.push(30U));

  std::uint8_t value = 0U;

  ASSERT_TRUE(buffer.pop(value));
  EXPECT_EQ(value, 10U);

  ASSERT_TRUE(buffer.pop(value));
  EXPECT_EQ(value, 20U);

  EXPECT_TRUE(buffer.push(40U));
  EXPECT_TRUE(buffer.push(50U));

  EXPECT_TRUE(buffer.full());

  ASSERT_TRUE(buffer.pop(value));
  EXPECT_EQ(value, 30U);

  ASSERT_TRUE(buffer.pop(value));
  EXPECT_EQ(value, 40U);

  ASSERT_TRUE(buffer.pop(value));
  EXPECT_EQ(value, 50U);

  EXPECT_TRUE(buffer.empty());
}

TEST(RingBufferTest, SupportsSingleElementCapacity) {
  RingBuffer<1> buffer;

  EXPECT_TRUE(buffer.push(42U));
  EXPECT_TRUE(buffer.full());
  EXPECT_FALSE(buffer.push(43U));

  std::uint8_t value = 0U;

  ASSERT_TRUE(buffer.pop(value));
  EXPECT_EQ(value, 42U);

  EXPECT_TRUE(buffer.empty());
}

} // namespace
