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

enum class AddressResult : std::uint8_t {
  acknowledged,
  not_acknowledged,
  timeout
};

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

AddressResult wait_for_address_response(volatile std::uint32_t &sr1) {
  for (std::uint32_t i = 0U; i < timeout_iterations; ++i) {
    if ((sr1 & mcu::i2c1::sr1_bit::address_sent) != 0U) {
      return AddressResult::acknowledged;
    }

    if ((sr1 & mcu::i2c1::sr1_bit::acknowledge_failure) != 0U) {
      return AddressResult::not_acknowledged;
    }
  }

  return AddressResult::timeout;
}

void finish_failed_transfer(volatile std::uint32_t &cr1,
                            volatile std::uint32_t &sr1) {
  if ((sr1 & mcu::i2c1::sr1_bit::acknowledge_failure) != 0U) {
    sr1 &= ~mcu::i2c1::sr1_bit::acknowledge_failure;
  }

  cr1 |= mcu::i2c1::cr1_bit::stop;
  cr1 |= mcu::i2c1::cr1_bit::acknowledge;
  cr1 &= ~mcu::i2c1::cr1_bit::acknowledge_position;
}

void restore_received_configuration(volatile std::uint32_t &cr1) {
  cr1 |= mcu::i2c1::cr1_bit::acknowledge;
  cr1 &= ~mcu::i2c1::cr1_bit::acknowledge_position;
}

bool receive_two_bytes(volatile std::uint32_t &cr1, volatile std::uint32_t &dr,
                       volatile std::uint32_t &sr1,
                       std::uint8_t *const buffer) {
  cr1 |= mcu::i2c1::cr1_bit::acknowledge_position;
  cr1 &= ~mcu::i2c1::cr1_bit::acknowledge;

  clear_addr_flag();

  if (!wait_until_set(sr1, mcu::i2c1::sr1_bit::byte_transfer_finished)) {
    cr1 |= mcu::i2c1::cr1_bit::stop;
    restore_received_configuration(cr1);
    return false;
  }
  cr1 |= mcu::i2c1::cr1_bit::stop;
  buffer[0] = static_cast<std::uint8_t>(dr & 0xFFU);
  buffer[1] = static_cast<std::uint8_t>(dr & 0xFFU);
  restore_received_configuration(cr1);
  return true;
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

ReadResult read_register(const std::uint8_t address,
                         const std::uint8_t register_address,
                         std::uint8_t &value) {
  auto &cr1 = mcu::reg(mcu::i2c1::cr1);
  auto &dr = mcu::reg(mcu::i2c1::dr);
  auto &sr1 = mcu::reg(mcu::i2c1::sr1);
  auto &sr2 = mcu::reg(mcu::i2c1::sr2);

  saved_sr1 = 0U;
  saved_sr2 = 0U;

  if (!wait_until_clear(sr2, mcu::i2c1::sr2_bit::bus_busy)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    return ReadResult::bus_busy_timeout;
  }

  cr1 |= mcu::i2c1::cr1_bit::acknowledge;
  cr1 |= mcu::i2c1::cr1_bit::start;

  if (!wait_until_set(sr1, mcu::i2c1::sr1_bit::start_generated)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    finish_failed_transfer(cr1, sr1);
    return ReadResult::start_timeout;
  }

  // Send the seven-bit sensor address with the write bit.
  dr = static_cast<std::uint32_t>(address) << 1U;

  auto address_result = wait_for_address_response(sr1);

  if (address_result != AddressResult::acknowledged) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    finish_failed_transfer(cr1, sr1);

    return address_result == AddressResult::not_acknowledged
               ? ReadResult::address_not_acknowledged
               : ReadResult::response_timeout;
  }

  clear_addr_flag();

  if (!wait_until_set(sr1, mcu::i2c1::sr1_bit::transmit_buffer_empty)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    finish_failed_transfer(cr1, sr1);
    return ReadResult::transmit_timeout;
  }

  dr = register_address;

  if (!wait_until_set(sr1, mcu::i2c1::sr1_bit::byte_transfer_finished)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    finish_failed_transfer(cr1, sr1);
    return ReadResult::transmit_timeout;
  }

  // Generate repeated START without releasing the bus.
  cr1 |= mcu::i2c1::cr1_bit::start;

  if (!wait_until_set(sr1, mcu::i2c1::sr1_bit::start_generated)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    finish_failed_transfer(cr1, sr1);
    return ReadResult::start_timeout;
  }

  // Send the sensor address again, now with the read bit.
  dr = (static_cast<std::uint32_t>(address) << 1U) | 1U;

  address_result = wait_for_address_response(sr1);

  if (address_result != AddressResult::acknowledged) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    finish_failed_transfer(cr1, sr1);

    return address_result == AddressResult::not_acknowledged
               ? ReadResult::address_not_acknowledged
               : ReadResult::response_timeout;
  }

  // STM32F4 one-byte master-receiver sequence:
  // disable ACK, clear ADDR, generate STOP, then read DR.
  cr1 &= ~mcu::i2c1::cr1_bit::acknowledge;
  clear_addr_flag();
  cr1 |= mcu::i2c1::cr1_bit::stop;

  if (!wait_until_set(sr1, mcu::i2c1::sr1_bit::receive_buffer_not_empty)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    cr1 |= mcu::i2c1::cr1_bit::acknowledge;
    return ReadResult::receive_timeout;
  }

  value = static_cast<std::uint8_t>(dr & 0xFFU);

  // Restore the default ACK state for subsequent transactions.
  cr1 |= mcu::i2c1::cr1_bit::acknowledge;

  saved_sr1 = sr1;
  saved_sr2 = sr2;

  return ReadResult::success;
}

ReadResult read_registers(const std::uint8_t address,
                          const std::uint8_t start_register,
                          std::uint8_t *const buffer,
                          const std::size_t length) {
  if (buffer == nullptr || length == 0U) {
    return ReadResult::invalid_argument;
  }

  if (length == 1U) {
    return read_register(address, start_register, buffer[0]);
  }

  if (length != 2U) {
    return ReadResult::invalid_argument;
  }
  auto &cr1 = mcu::reg(mcu::i2c1::cr1);
  auto &dr = mcu::reg(mcu::i2c1::dr);
  auto &sr1 = mcu::reg(mcu::i2c1::sr1);
  auto &sr2 = mcu::reg(mcu::i2c1::sr2);

  saved_sr1 = 0U;
  saved_sr2 = 0U;

  if (!wait_until_clear(sr2, mcu::i2c1::sr2_bit::bus_busy)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    return ReadResult::bus_busy_timeout;
  }
  restore_received_configuration(cr1);
  cr1 |= mcu::i2c1::cr1_bit::start;

  if (!wait_until_set(sr1, mcu::i2c1::sr1_bit::start_generated)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    finish_failed_transfer(cr1, sr1);
    return ReadResult::start_timeout;
  }

  dr = static_cast<std::uint32_t>(address) << 1U;

  auto address_result = wait_for_address_response(sr1);

  if (address_result != AddressResult::acknowledged) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    finish_failed_transfer(cr1, sr1);

    return address_result == AddressResult::not_acknowledged
               ? ReadResult::address_not_acknowledged
               : ReadResult::response_timeout;
  }
  clear_addr_flag();

  if (!wait_until_set(sr1, mcu::i2c1::sr1_bit::transmit_buffer_empty)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    finish_failed_transfer(cr1, sr1);
    return ReadResult::transmit_timeout;
  }
  dr = start_register;

  if (!wait_until_set(sr1, mcu::i2c1::sr1_bit::byte_transfer_finished)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    finish_failed_transfer(cr1, sr1);
    return ReadResult::transmit_timeout;
  }

  // Generate repeated START without releasing the bus.
  cr1 |= mcu::i2c1::cr1_bit::start;
  if (!wait_until_set(sr1, mcu::i2c1::sr1_bit::start_generated)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    finish_failed_transfer(cr1, sr1);
    return ReadResult::start_timeout;
  }

  // Send the sensor address again, now with the read bit.
  dr = (static_cast<std::uint32_t>(address) << 1U) | 1U;

  address_result = wait_for_address_response(sr1);

  if (address_result != AddressResult::acknowledged) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    finish_failed_transfer(cr1, sr1);

    return address_result == AddressResult::not_acknowledged
               ? ReadResult::address_not_acknowledged
               : ReadResult::response_timeout;
  }

  if (!receive_two_bytes(cr1, dr, sr1, buffer)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    return ReadResult::receive_timeout;
  }

  saved_sr1 = sr1;
  saved_sr2 = sr2;
  return ReadResult::success;
}

WriteResult write_register(const std::uint8_t address,
                           const std::uint8_t register_address,
                           const std::uint8_t value) {
  auto &cr1 = mcu::reg(mcu::i2c1::cr1);
  auto &dr = mcu::reg(mcu::i2c1::dr);
  auto &sr1 = mcu::reg(mcu::i2c1::sr1);
  auto &sr2 = mcu::reg(mcu::i2c1::sr2);

  saved_sr1 = 0U;
  saved_sr2 = 0U;

  if (!wait_until_clear(sr2, mcu::i2c1::sr2_bit::bus_busy)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    return WriteResult::bus_busy_timeout;
  }

  cr1 |= mcu::i2c1::cr1_bit::start;

  if (!wait_until_set(sr1, mcu::i2c1::sr1_bit::start_generated)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    finish_failed_transfer(cr1, sr1);
    return WriteResult::start_timeout;
  }

  // Send the seven-bit sensor address with the write bit (0).
  dr = static_cast<std::uint32_t>(address) << 1U;

  const auto address_result = wait_for_address_response(sr1);

  if (address_result != AddressResult::acknowledged) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    finish_failed_transfer(cr1, sr1);

    return address_result == AddressResult::not_acknowledged
               ? WriteResult::address_not_acknowledged
               : WriteResult::response_timeout;
  }

  clear_addr_flag();

  if (!wait_until_set(sr1, mcu::i2c1::sr1_bit::transmit_buffer_empty)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    finish_failed_transfer(cr1, sr1);
    return WriteResult::transmit_timeout;
  }

  // Select the sensor register to write to.
  dr = register_address;

  if (!wait_until_set(sr1, mcu::i2c1::sr1_bit::transmit_buffer_empty)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    finish_failed_transfer(cr1, sr1);
    return WriteResult::transmit_timeout;
  }

  // Write the new register value.
  dr = value;

  if (!wait_until_set(sr1, mcu::i2c1::sr1_bit::byte_transfer_finished)) {
    saved_sr1 = sr1;
    saved_sr2 = sr2;
    finish_failed_transfer(cr1, sr1);
    return WriteResult::transmit_timeout;
  }

  cr1 |= mcu::i2c1::cr1_bit::stop;

  saved_sr1 = sr1;
  saved_sr2 = sr2;

  return WriteResult::success;
}

std::uint32_t last_sr1() { return saved_sr1; }

std::uint32_t last_sr2() { return saved_sr2; }

} // namespace drivers::i2c1