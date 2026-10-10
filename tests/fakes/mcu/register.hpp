#pragma once

#include <cstdint>
#include <map>

// Minimal synchronous MMIO model for usart2_dma_tests only. It models register
// side effects, not DMA bus timing, byte delivery, or NVIC exception scheduling.
namespace fake_mcu {
inline std::map<std::uintptr_t, std::uint32_t> values;
inline bool stuck_enable = false;
inline bool stuck_flags = false;
inline std::uint32_t stream_reads = 0U;
inline std::uint32_t writes = 0U;

inline std::uint32_t read(std::uintptr_t address) {
  if (address == 0x400260A0U) {
    ++stream_reads;
  }
  return values[address];
}

inline void write(std::uintptr_t address, std::uint32_t value) {
  ++writes;
  if (address == 0x4002600CU) { // HIFCR: write one to clear HISR.
    if (!stuck_flags) values[0x40026004U] &= ~value;
    return;
  }
  if (address >= 0xE000E100U && address < 0xE000E108U) {
    values[address] |= value; // ISER: write one to enable.
    return;
  }
  if (address >= 0xE000E180U && address < 0xE000E188U) {
    values[address - 0x80U] &= ~value; // ICER: write one to disable.
    return;
  }
  if (address == 0x400260A0U && stuck_enable) {
    value |= values[address] & 1U; // Refuse to clear an already set EN.
  }
  values[address] = value;
}

struct Register {
  std::uintptr_t address;
  operator std::uint32_t() const { return read(address); }
  Register &operator=(std::uint32_t value) {
    write(address, value);
    return *this;
  }
};
inline std::map<std::uintptr_t, Register> registers;
inline void reset() {
  values.clear();
  registers.clear();
  stuck_enable = stuck_flags = false;
  stream_reads = writes = 0U;
}
inline bool irq_enabled(std::uint32_t irq) {
  return (values[0xE000E100U + (irq / 32U) * 4U] &
          (1U << (irq % 32U))) != 0U;
}
} // namespace fake_mcu

namespace mcu {
inline fake_mcu::Register &reg(std::uintptr_t address) {
  return fake_mcu::registers.try_emplace(address, fake_mcu::Register{address})
      .first->second;
}
inline void set_bits(std::uintptr_t address, std::uint32_t mask) {
  reg(address) = static_cast<std::uint32_t>(reg(address)) | mask;
}
inline void clear_bits(std::uintptr_t address, std::uint32_t mask) {
  reg(address) = static_cast<std::uint32_t>(reg(address)) & ~mask;
}
inline void modify(std::uintptr_t address, std::uint32_t clear_mask,
                   std::uint32_t set_mask) {
  reg(address) = (static_cast<std::uint32_t>(reg(address)) & ~clear_mask) | set_mask;
}
} // namespace mcu
