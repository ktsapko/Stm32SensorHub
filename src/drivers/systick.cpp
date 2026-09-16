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

void wait_for_tick() {
  auto &control = mcu::reg(mcu::systick::control);

  while ((control & mcu::systick::control_bit::count_flag) == 0U) {
  }
}

} // namespace

void initialize() {
  auto &control = mcu::reg(mcu::systick::control);
  auto &reload = mcu::reg(mcu::systick::reload);
  auto &current = mcu::reg(mcu::systick::current);

  control = 0U;
  reload = reload_value;
  current = 0U;

  control = mcu::systick::control_bit::processor_clock |
            mcu::systick::control_bit::enable;
}

void delay_ms(const std::uint32_t milliseconds) {
  for (std::uint32_t elapsed = 0U; elapsed < milliseconds; ++elapsed) {
    wait_for_tick();
  }
}

} // namespace drivers::systick