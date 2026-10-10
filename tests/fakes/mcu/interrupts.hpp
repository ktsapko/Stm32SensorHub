#pragma once
#include <cstdint>

namespace mcu::interrupts {
// Host bookkeeping only; this does not simulate interrupt preemption.
inline std::uint32_t primask = 0U;
inline std::uint32_t save_and_disable() {
  const auto previous = primask;
  primask = 1U;
  return previous;
}
inline void restore(std::uint32_t previous) { primask = previous; }
} // namespace mcu::interrupts
