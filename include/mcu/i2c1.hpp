#pragma once

#include <cstdint>

namespace mcu::i2c1 {

constexpr std::uintptr_t base = 0x40005400U;

constexpr std::uintptr_t cr1 = base + 0x00U;
constexpr std::uintptr_t cr2 = base + 0x04U;
constexpr std::uintptr_t oar1 = base + 0x08U;
constexpr std::uintptr_t dr = base + 0x10U;
constexpr std::uintptr_t sr1 = base + 0x14U;
constexpr std::uintptr_t sr2 = base + 0x18U;
constexpr std::uintptr_t ccr = base + 0x1CU;
constexpr std::uintptr_t trise = base + 0x20U;

namespace cr1_bit {

constexpr std::uint32_t peripheral_enable = 1U << 0U;
constexpr std::uint32_t start = 1U << 8U;
constexpr std::uint32_t stop = 1U << 9U;
constexpr std::uint32_t acknowledge = 1U << 10U;
constexpr std::uint32_t acknowledge_position = 1U << 11U;
constexpr std::uint32_t software_reset = 1U << 15U;

} // namespace cr1_bit

namespace sr1_bit {

constexpr std::uint32_t start_generated = 1U << 0U;
constexpr std::uint32_t address_sent = 1U << 1U;
constexpr std::uint32_t byte_transfer_finished = 1U << 2U;
constexpr std::uint32_t receive_buffer_not_empty = 1U << 6U;
constexpr std::uint32_t transmit_buffer_empty = 1U << 7U;
constexpr std::uint32_t acknowledge_failure = 1U << 10U;

} // namespace sr1_bit

namespace sr2_bit {

constexpr std::uint32_t bus_busy = 1U << 1U;

} // namespace sr2_bit

} // namespace mcu::i2c1