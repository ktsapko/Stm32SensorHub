#include "drivers/usart2.hpp"   

#include <cstdint>

namespace {

constexpr std::uintptr_t rcc_ahb1enr_address = 0x40023830U;
constexpr std::uintptr_t rcc_apb1enr_address = 0x40023840U;

constexpr std::uintptr_t gpioa_moder_address = 0x40020000U;
constexpr std::uintptr_t gpioa_afrl_address = 0x40020020U;

constexpr std::uintptr_t usart2_sr_address = 0x40004400U;
constexpr std::uintptr_t usart2_dr_address = 0x40004404U;
constexpr std::uintptr_t usart2_brr_address = 0x40004408U;
constexpr std::uintptr_t usart2_cr1_address = 0x4000440CU;

constexpr std::uint32_t gpioa_clock_enable = 1U << 0U;
constexpr std::uint32_t usart2_clock_enable = 1U << 17U;

constexpr std::uint32_t usart_enable = 1U << 13U;
constexpr std::uint32_t transmitter_enable = 1U << 3U;
constexpr std::uint32_t transmit_data_register_empty = 1U << 7U;

constexpr std::uint32_t tx_pin = 2U;
constexpr std::uint32_t alternate_function_7 = 7U;

/*
 * Після reset STM32F401 працює від HSI 16 MHz.
 *
 * USARTDIV = 16'000'000 / 115'200 ≈ 138.89
 * BRR = 139 = 0x008B
 */
constexpr std::uint32_t baud_rate_register = 0x008BU;

volatile std::uint32_t& register_at(const std::uintptr_t address)
{
    return *reinterpret_cast<volatile std::uint32_t*>(address);
}

} // namespace

namespace drivers::usart2 {

void initialize()
{
    auto& rcc_ahb1enr = register_at(rcc_ahb1enr_address);
    rcc_ahb1enr = rcc_ahb1enr | gpioa_clock_enable;

    auto& rcc_apb1enr = register_at(rcc_apb1enr_address);
    rcc_apb1enr = rcc_apb1enr | usart2_clock_enable;

    /*
     * PA2 переводимо в Alternate Function mode: MODER2 = 10.
     */
    auto& gpioa_moder = register_at(gpioa_moder_address);
    gpioa_moder =
        (gpioa_moder & ~(0b11U << (tx_pin * 2U))) |
        (0b10U << (tx_pin * 2U));

    /*
     * PA2 використовує AF7, тобто USART2_TX.
     */
    auto& gpioa_afrl = register_at(gpioa_afrl_address);
    gpioa_afrl =
        (gpioa_afrl & ~(0b1111U << (tx_pin * 4U))) |
        (alternate_function_7 << (tx_pin * 4U));

    register_at(usart2_brr_address) = baud_rate_register;
    register_at(usart2_cr1_address) =
        usart_enable | transmitter_enable;
}

void write_byte(const char byte)
{
    auto& usart2_sr = register_at(usart2_sr_address);

    while ((usart2_sr & transmit_data_register_empty) == 0U) {
        // Чекаємо, поки USART зможе прийняти наступний байт.
    }

    register_at(usart2_dr_address) =
        static_cast<std::uint32_t>(
            static_cast<unsigned char>(byte));
}

void write(const char* text)
{
    while (*text != '\0') {
        write_byte(*text);
        ++text;
    }
}

} // namespace drivers::usart2