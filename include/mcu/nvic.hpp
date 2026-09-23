
#pragma once

#include "mcu/register.hpp"

#include <cstdint>

namespace mcu::nvic {

constexpr std::uintptr_t iser_base = 0xE000E100U;

constexpr std::uint32_t usart2_irq = 38U;

inline void enable_irq(const std::uint32_t irq) {
  constexpr std::uint32_t bits_per_register = 32U;
  constexpr std::uint32_t register_stride = 4U;

  const auto register_index = irq / bits_per_register;
  const auto bit_index = irq % bits_per_register;

  const auto address = iser_base + register_index * register_stride;

  mcu::reg(address) = 1U << bit_index;
}

} // namespace mcu::nvic
