
#pragma once

#include "mcu/register.hpp"

#include <cstdint>

namespace mcu::nvic {

constexpr std::uintptr_t iser_base = 0xE000E100U;
constexpr std::uintptr_t icer_base = 0xE000E180U;
constexpr std::uintptr_t ipr_base = 0xE000E400U;

constexpr std::uint32_t usart2_irq = 38U;
constexpr std::uint32_t dma1_stream6_irq = 17U;

// STM32F401: logical priority 0..15 occupies the upper four bits.
inline void set_priority(const std::uint32_t irq, const std::uint8_t priority) {
  auto &value = mcu::reg8(ipr_base + irq);
  value = static_cast<std::uint8_t>(priority << 4U);
}

inline void enable_irq(const std::uint32_t irq) {
  constexpr std::uint32_t bits_per_register = 32U;
  constexpr std::uint32_t register_stride = 4U;

  const auto register_index = irq / bits_per_register;
  const auto bit_index = irq % bits_per_register;

  const auto address = iser_base + register_index * register_stride;

  mcu::reg(address) = 1U << bit_index;
}

inline void disable_irq(const std::uint32_t irq) {
  constexpr std::uint32_t bits_per_register = 32U;
  constexpr std::uint32_t register_stride = 4U;

  const auto register_index = irq / bits_per_register;
  const auto bit_index = irq % bits_per_register;

  const auto address = icer_base + register_index * register_stride;

  mcu::reg(address) = 1U << bit_index;
}

} // namespace mcu::nvic
