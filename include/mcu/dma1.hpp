#pragma once

#include <cstdint>

namespace mcu::dma1 {

constexpr std::uintptr_t base = 0x40026000U;

constexpr std::uintptr_t hisr = base + 0x0004U;
constexpr std::uintptr_t hifcr = base + 0x000CU;

constexpr std::uintptr_t s6cr = base + 0x00A0U;
constexpr std::uintptr_t s6ndtr = base + 0x00A4U;
constexpr std::uintptr_t s6par = base + 0x00A8U;
constexpr std::uintptr_t s6m0ar = base + 0x00ACU;
constexpr std::uintptr_t s6m1ar = base + 0x00B0U;
constexpr std::uintptr_t s6fcr = base + 0x00B4U;

namespace cr_bit {
constexpr std::uint32_t enable = 1U << 0U;
constexpr std::uint32_t memory_increment = 1U << 10U;
constexpr std::uint32_t transfer_complete_interrupt = 1U << 4U;
constexpr std::uint32_t transfer_error_interrupt = 1U << 2U;
constexpr std::uint32_t direct_mode_error_interrupt = 1U << 1U;
} // namespace cr_bit

namespace cr_field {
constexpr std::uint32_t channel_4 = 4U << 25U;
constexpr std::uint32_t memory_to_peripheral = 1U << 6U;
} // namespace cr_field

namespace hisr_bit {
constexpr std::uint32_t tcif6 = 1U << 21U;
constexpr std::uint32_t htif6 = 1U << 20U;
constexpr std::uint32_t teif6 = 1U << 19U;
constexpr std::uint32_t dmeif6 = 1U << 18U;
constexpr std::uint32_t feif6 = 1U << 16U;
} // namespace hisr_bit

namespace hifcr_bit {
constexpr std::uint32_t ctcif6 = 1U << 21U;
constexpr std::uint32_t chtif6 = 1U << 20U;
constexpr std::uint32_t cteif6 = 1U << 19U;
constexpr std::uint32_t cdmeif6 = 1U << 18U;
constexpr std::uint32_t cfeif6 = 1U << 16U;
} // namespace hifcr_bit

} // namespace mcu::dma1
