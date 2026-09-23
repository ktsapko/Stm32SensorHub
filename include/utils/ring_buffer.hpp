
#pragma once

#include <cstddef>
#include <cstdint>

namespace utils {

template <std::size_t Capacity> class RingBuffer {
  static_assert(Capacity > 0U);

public:
  bool push(const std::uint8_t value) {
    if (full()) {
      return false;
    }

    buffer_[head_] = value;
    head_ = next(head_);
    ++size_;

    return true;
  }

  bool pop(std::uint8_t &value) {
    if (empty()) {
      return false;
    }

    value = buffer_[tail_];
    tail_ = next(tail_);
    --size_;

    return true;
  }

  [[nodiscard]] bool empty() const { return size_ == 0U; }

  [[nodiscard]] bool full() const { return size_ == Capacity; }

  [[nodiscard]] std::size_t size() const { return size_; }

  [[nodiscard]] constexpr std::size_t capacity() const { return Capacity; }

private:
  static constexpr std::size_t next(const std::size_t index) {
    return (index + 1U) % Capacity;
  }

  std::uint8_t buffer_[Capacity]{};

  std::size_t head_ = 0U;
  std::size_t tail_ = 0U;
  std::size_t size_ = 0U;
};

} // namespace utils
