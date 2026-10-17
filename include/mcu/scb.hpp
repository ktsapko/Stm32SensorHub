#pragma once

#include "mcu/register.hpp"

#include <cstdint>

namespace mcu::scb {

constexpr std::uintptr_t aircr = 0xE000ED0CU;

inline void use_all_preemption_priority_bits() {
  constexpr std::uint32_t vectkey = 0x5FAU << 16U;
  // Preserve ENDIANNESS (read-only); write zero to PRIGROUP and reset requests.
  constexpr std::uint32_t endianness = 1U << 15U;
  mcu::reg(aircr) = vectkey | (mcu::reg(aircr) & endianness);
  asm volatile("dsb\nisb" ::: "memory");
}

} // namespace mcu::scb
