#pragma once

#include <cstdint>

namespace mcu {

inline volatile std::uint32_t &reg(const std::uintptr_t address) {
  return *reinterpret_cast<volatile std::uint32_t *>(address);
}

inline void set_bits(const std::uintptr_t address, const std::uint32_t mask) {
  reg(address) |= mask;
}

inline void clear_bits(const std::uintptr_t address, const std::uint32_t mask) {
  reg(address) &= ~mask;
}

inline void modify(const std::uintptr_t address, const std::uint32_t clear_mask,
                   const std::uint32_t set_mask) {
  auto &value = reg(address);
  value = (value & ~clear_mask) | set_mask;
}

} // namespace mcu