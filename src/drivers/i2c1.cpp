#include "drivers/i2c1.hpp"
#include "mcu/gpio.hpp"
#include "mcu/i2c1.hpp"
#include "mcu/rcc.hpp"
#include "mcu/register.hpp"

#include <cstdint>
#include <initializer_list>

namespace drivers::i2c1 {
namespace {

constexpr std::uint32_t timeout_iterations = 100'000U;

std::uint32_t saved_sr1 = 0U;
std::uint32_t saved_sr2 = 0U;

bool wait_until_set(volatile std::uint32_t &reg, const std::uint32_t mask) {
  for (std::uint32_t i = 0U; i < timeout_iterations; ++i) {
    if ((reg & mask) != 0U) {
      return true;
    }
  }

  return false;
}

bool wait_until_clear(volatile std::uint32_t &reg, const std::uint32_t mask) {
  for (std::uint32_t i = 0U; i < timeout_iterations; ++i) {
    if ((reg & mask) == 0U) {
      return true;
    }
  }

  return false;
}

void configure_gpio() {
  constexpr std::uint32_t scl_pin = 8U;
  constexpr std::uint32_t sda_pin = 9U;
  constexpr std::uint32_t i2c_alternate_function = 4U;

  for (const auto pin : {scl_pin, sda_pin}) {
    mcu::gpio::set_open_drain(mcu::gpio::gpiob, pin);
    mcu::gpio::set_speed(mcu::gpio::gpiob, pin, mcu::gpio::Speed::fast);
    mcu::gpio::set_pull(mcu::gpio::gpiob, pin, mcu::gpio::Pull::none);
    mcu::gpio::set_alternate_function(mcu::gpio::gpiob, pin,
                                      i2c_alternate_function);
    mcu::gpio::set_mode(mcu::gpio::gpiob, pin, mcu::gpio::Mode::alternate);
  }
}

void clear_addr_flag() {
  const auto sr1 = mcu::reg(mcu::i2c1::sr1);
  const auto sr2 = mcu::reg(mcu::i2c1::sr2);

  (void)sr1;
  (void)sr2;
}

} // namespace

void initialize() {

  mcu::rcc::enable_ahb1(mcu::rcc::ahb1::gpiob);
  mcu::rcc::enable_apb1(mcu::rcc::apb1::i2c1);
  mcu::rcc::reset_apb1(mcu::rcc::apb1::i2c1);

  configure_gpio();

  auto &cr1 = mcu::reg(mcu::i2c1::cr1);

  cr1 |= mcu::i2c1::cr1_bit::software_reset;
  cr1 &= ~mcu::i2c1::cr1_bit::software_reset;

  auto &cr2 = mcu::reg(mcu::i2c1::cr2);
  auto &oar1 = mcu::reg(mcu::i2c1::oar1);
  auto &ccr = mcu::reg(mcu::i2c1::ccr);
  auto &trise = mcu::reg(mcu::i2c1::trise);

  cr1 &= ~mcu::i2c1::cr1_bit::peripheral_enable;

  cr2 = 16U;
  ccr = 80U;
  trise = 17U;
  oar1 = 1U << 14U;

  saved_sr1 = 0U;
  saved_sr2 = 0U;

  cr1 |= mcu::i2c1::cr1_bit::peripheral_enable;
}

ProbeResult probe(const std::uint8_t address) {
  auto &cr1 = mcu::reg(mcu::i2c1::cr1);
  auto &dr = mcu::reg(mcu::i2c1::dr);
  auto &sr1 = mcu::reg(mcu::i2c1::sr1);
  auto &sr2 = mcu::reg(mcu::i2c1::sr2);

  saved_sr1 = 0U;
  saved_sr2 = 0U;

  if (!wait_until_clear(sr2, mcu::i2c1::sr2_bit::bus_busy)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    return ProbeResult::bus_busy_timeout;
  }

  cr1 |= mcu::i2c1::cr1_bit::start;

  if (!wait_until_set(sr1, mcu::i2c1::sr1_bit::start_generated)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    cr1 |= mcu::i2c1::cr1_bit::stop;
    return ProbeResult::start_timeout;
  }

  // Seven-bit address followed by the write bit (0).
  dr = static_cast<std::uint32_t>(address) << 1U;

  for (std::uint32_t i = 0U; i < timeout_iterations; ++i) {
    if ((sr1 & mcu::i2c1::sr1_bit::address_sent) != 0U) {
      saved_sr1 = sr1;
      saved_sr2 = sr2;

      clear_addr_flag();
      cr1 |= mcu::i2c1::cr1_bit::stop;

      return ProbeResult::acknowledged;
    }

    if ((sr1 & mcu::i2c1::sr1_bit::acknowledge_failure) != 0U) {
      saved_sr1 = sr1;
      saved_sr2 = sr2;

      sr1 &= ~mcu::i2c1::sr1_bit::acknowledge_failure;
      cr1 |= mcu::i2c1::cr1_bit::stop;

      return ProbeResult::not_acknowledged;
    }
  }

  saved_sr1 = sr1;
  saved_sr2 = sr2;

  cr1 |= mcu::i2c1::cr1_bit::stop;

  return ProbeResult::response_timeout;
}

std::uint32_t last_sr1() { return saved_sr1; }

std::uint32_t last_sr2() { return saved_sr2; }

} // namespace drivers::i2c1