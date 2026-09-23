#pragma once

#include <cstdint>

namespace mcu::usart2 {

constexpr std::uintptr_t base = 0x40004400U;

constexpr std::uintptr_t sr = base + 0x00U;
constexpr std::uintptr_t dr = base + 0x04U;
constexpr std::uintptr_t brr = base + 0x08U;
constexpr std::uintptr_t cr1 = base + 0x0CU;

namespace sr_bit {

constexpr std::uint32_t transmit_data_register_empty = 1U << 7U;

} // namespace sr_bit

namespace cr1_bit {

constexpr std::uint32_t transmitter_enable = 1U << 3U;
constexpr std::uint32_t usart_enable = 1U << 13U;
constexpr std::uint32_t transmit_data_register_empty_interrupt = 1U << 7U;

} // namespace cr1_bit

} // namespace mcu::usart2