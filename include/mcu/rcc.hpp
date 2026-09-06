#pragma once

#include "mcu/register.hpp"

#include <cstdint>

namespace mcu::rcc {

constexpr std::uintptr_t apb1rstr = 0x40023820U;
constexpr std::uintptr_t ahb1enr = 0x40023830U;
constexpr std::uintptr_t apb1enr = 0x40023840U;

namespace ahb1 {

constexpr std::uint32_t gpioa = 1U << 0U;
constexpr std::uint32_t gpiob = 1U << 1U;

} // namespace ahb1

namespace apb1 {

constexpr std::uint32_t usart2 = 1U << 17U;
constexpr std::uint32_t i2c1 = 1U << 21U;

} // namespace apb1

inline void enable_ahb1(const std::uint32_t peripheral) {
  set_bits(ahb1enr, peripheral);

  const auto read_back = reg(ahb1enr);
  (void)read_back;
}

inline void enable_apb1(const std::uint32_t peripheral) {
  set_bits(apb1enr, peripheral);

  const auto read_back = reg(apb1enr);
  (void)read_back;
}

inline void reset_apb1(const std::uint32_t peripheral) {
  set_bits(apb1rstr, peripheral);
  clear_bits(apb1rstr, peripheral);
}

} // namespace mcu::rcc