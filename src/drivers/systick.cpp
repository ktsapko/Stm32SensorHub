#include "drivers/systick.hpp"

#include "mcu/register.hpp"
#include "mcu/systick.hpp"

#include <cstdint>

namespace drivers::systick {
namespace {

constexpr std::uint32_t system_clock_hz = 16'000'000U;
constexpr std::uint32_t milliseconds_per_second = 1'000U;
constexpr std::uint32_t ticks_per_millisecond =
    system_clock_hz / milliseconds_per_second;
constexpr std::uint32_t reload_value = ticks_per_millisecond - 1U;

static_assert(reload_value <= mcu::systick::maximum_reload);

volatile std::uint32_t milliseconds_counter = 0U;

} // namespace

void initialize() {
  auto &control = mcu::reg(mcu::systick::control);
  auto &reload = mcu::reg(mcu::systick::reload);
  auto &current = mcu::reg(mcu::systick::current);

  control = 0U;
  reload = reload_value;
  current = 0U;
  milliseconds_counter = 0U;

  control = mcu::systick::control_bit::processor_clock |
            mcu::systick::control_bit::interrupt |
            mcu::systick::control_bit::enable;
}

std::uint32_t milliseconds() { return milliseconds_counter; }

void delay_ms(const std::uint32_t duration_ms) {
  const std::uint32_t start = milliseconds();

  while ((milliseconds() - start) < duration_ms) {
    asm volatile("wfi");
  }
}

void handle_interrupt() { milliseconds_counter = milliseconds_counter + 1U; }

} // namespace drivers::systick

extern "C" void SysTick_Handler() { drivers::systick::handle_interrupt(); }