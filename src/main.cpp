#include "drivers/usart2.hpp"

#include <cstdint>

namespace {

constexpr std::uintptr_t rcc_ahb1enr_address = 0x40023830U;
constexpr std::uintptr_t gpioa_moder_address = 0x40020000U;
constexpr std::uintptr_t gpioa_bsrr_address = 0x40020018U;

constexpr std::uint32_t gpioa_clock_enable = 1U << 0U;
constexpr std::uint32_t led_pin = 5U;

volatile std::uint32_t& register_at(const std::uintptr_t address)
{
    return *reinterpret_cast<volatile std::uint32_t*>(address);
}

void delay(const std::uint32_t cycles)
{
    for (std::uint32_t i = 0; i < cycles; ++i) {
        asm volatile("nop");
    }
}

void initialize_led()
{
    auto& rcc_ahb1enr = register_at(rcc_ahb1enr_address);
    rcc_ahb1enr = rcc_ahb1enr | gpioa_clock_enable;

    /* PA5: MODER5 = 01, general-purpose output */
    auto& gpioa_moder = register_at(gpioa_moder_address);
    gpioa_moder =
        (gpioa_moder & ~(0b11U << (led_pin * 2U))) |
        (0b01U << (led_pin * 2U));
}

void set_led(const bool enabled)
{
    auto& gpioa_bsrr = register_at(gpioa_bsrr_address);

    if (enabled) {
        gpioa_bsrr = 1U << led_pin;
    } else {
        gpioa_bsrr = 1U << (led_pin + 16U);
    }
}

} // namespace

int main()
{
    initialize_led();

    drivers::usart2::initialize();
    drivers::usart2::write("Stm32SensorHub started\r\n");

    while (true) {
        set_led(true);
        delay(2'000'000U);
        set_led(false);
        delay(2'000'000U);
    }
}