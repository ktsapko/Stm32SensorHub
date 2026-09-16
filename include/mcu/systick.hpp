#pragma once

#include <cstdint>

namespace mcu::systick {

constexpr std::uintptr_t control = 0xE000E010U;
constexpr std::uintptr_t reload = 0xE000E014U;
constexpr std::uintptr_t current = 0xE000E018U;

namespace control_bit {

constexpr std::uint32_t enable = 1U << 0U;
constexpr std::uint32_t interrupt = 1U << 1U;
constexpr std::uint32_t processor_clock = 1U << 2U;
constexpr std::uint32_t count_flag = 1U << 16U;

} // namespace control_bit

constexpr std::uint32_t maximum_reload = (1U << 24U) - 1U;

} // namespace mcu::systick