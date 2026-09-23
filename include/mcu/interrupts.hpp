
#pragma once

#include <cstdint>

namespace mcu::interrupts {

inline std::uint32_t save_and_disable() {
  std::uint32_t primask;

  asm volatile("mrs %0, primask\n"
               "cpsid i"
               : "=r"(primask)
               :
               : "memory");

  return primask;
}

inline void restore(const std::uint32_t primask) {
  asm volatile("msr primask, %0" : : "r"(primask) : "memory");
}

} // namespace mcu::interrupts
