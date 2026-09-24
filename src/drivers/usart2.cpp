
#include "drivers/usart2.hpp"

#include "mcu/gpio.hpp"
#include "mcu/interrupts.hpp"
#include "mcu/nvic.hpp"
#include "mcu/rcc.hpp"
#include "mcu/register.hpp"
#include "mcu/usart2.hpp"
#include "utils/ring_buffer.hpp"

#include <cstddef>
#include <cstdint>

namespace {

constexpr std::uint32_t tx_pin = 2U;
constexpr std::uint32_t rx_pin = 3U;
constexpr std::uint32_t usart_alternate_function = 7U;
constexpr std::uint32_t baud_rate_register = 0x008BU;
constexpr std::size_t rx_buffer_capacity = 512U;
constexpr std::size_t tx_buffer_capacity = 512U;

std::uint32_t dropped_bytes = 0U;
std::uint32_t dropped_rx = 0U;
std::uint32_t hardware_overruns = 0U;

utils::RingBuffer<tx_buffer_capacity> tx_buffer;
utils::RingBuffer<rx_buffer_capacity> rx_buffer;

void configure_tx_pin() {
  mcu::gpio::set_alternate_function(mcu::gpio::gpioa, tx_pin,
                                    usart_alternate_function);

  mcu::gpio::set_mode(mcu::gpio::gpioa, tx_pin, mcu::gpio::Mode::alternate);
}

void configure_rx_pin() {
  mcu::gpio::set_alternate_function(mcu::gpio::gpioa, rx_pin,
                                    usart_alternate_function);

  mcu::gpio::set_mode(mcu::gpio::gpioa, rx_pin, mcu::gpio::Mode::alternate);
}

void enable_tx_interrupt() {
  mcu::set_bits(mcu::usart2::cr1,
                mcu::usart2::cr1_bit::transmit_data_register_empty_interrupt);
}

void disable_tx_interrupt() {
  mcu::clear_bits(mcu::usart2::cr1,
                  mcu::usart2::cr1_bit::transmit_data_register_empty_interrupt);
}

void enable_rx_interrupt() {
  mcu::set_bits(
      mcu::usart2::cr1,
      mcu::usart2::cr1_bit::receive_data_register_not_empty_interrupt);
}
} // namespace

namespace drivers::usart2 {

void initialize() {
  mcu::rcc::enable_ahb1(mcu::rcc::ahb1::gpioa);
  mcu::rcc::enable_apb1(mcu::rcc::apb1::usart2);

  configure_tx_pin();
  configure_rx_pin();

  mcu::reg(mcu::usart2::brr) = baud_rate_register;

  mcu::reg(mcu::usart2::cr1) = mcu::usart2::cr1_bit::usart_enable |
                               mcu::usart2::cr1_bit::transmitter_enable |
                               mcu::usart2::cr1_bit::receiver_enable;

  enable_rx_interrupt();
  mcu::nvic::enable_irq(mcu::nvic::usart2_irq);
}

bool write_byte(const char byte) {
  const auto primask = mcu::interrupts::save_and_disable();

  const bool accepted = tx_buffer.push(static_cast<std::uint8_t>(byte));

  if (accepted) {
    enable_tx_interrupt();
  } else {
    ++dropped_bytes;
  }

  mcu::interrupts::restore(primask);

  return accepted;
}

std::size_t write(const char *text) {
  if (text == nullptr) {
    return 0U;
  }

  std::size_t accepted = 0U;

  const auto primask = mcu::interrupts::save_and_disable();

  while (*text != '\0') {
    if (!tx_buffer.push(static_cast<std::uint8_t>(*text))) {
      do {
        ++dropped_bytes;
        ++text;
      } while (*text != '\0');

      break;
    }

    ++accepted;
    ++text;
  }

  if (accepted != 0U) {
    enable_tx_interrupt();
  }

  mcu::interrupts::restore(primask);

  return accepted;
}

void handle_tx_interrupt() {
  if ((mcu::reg(mcu::usart2::sr) &
       mcu::usart2::sr_bit::transmit_data_register_empty) == 0U) {
    return;
  }

  std::uint8_t byte = 0U;

  if (tx_buffer.pop(byte)) {
    mcu::reg(mcu::usart2::dr) = byte;
  } else {
    disable_tx_interrupt();
  }
}

std::uint32_t dropped_tx_bytes() {
  const auto primask = mcu::interrupts::save_and_disable();

  const auto count = dropped_bytes;

  mcu::interrupts::restore(primask);

  return count;
}

void handle_rx_interrupt() {
  const auto status = mcu::reg(mcu::usart2::sr);

  if ((status & (mcu::usart2::sr_bit::receive_data_register_not_empty |
                 mcu::usart2::sr_bit::overrun_error)) == 0U) {
    return;
  }

  // Reading SR followed by DR clears RXNE and ORE.
  const auto byte = static_cast<std::uint8_t>(mcu::reg(mcu::usart2::dr));

  if ((status & mcu::usart2::sr_bit::overrun_error) != 0U) {
    ++hardware_overruns;
  }

  if ((status & mcu::usart2::sr_bit::receive_data_register_not_empty) != 0U &&
      !rx_buffer.push(byte)) {
    ++dropped_rx;
  }
}

bool read_byte(std::uint8_t &byte) {
  const auto primask = mcu::interrupts::save_and_disable();

  const bool available = rx_buffer.pop(byte);

  mcu::interrupts::restore(primask);

  return available;
}

std::uint32_t dropped_rx_bytes() {
  const auto primask = mcu::interrupts::save_and_disable();

  const auto count = dropped_rx;

  mcu::interrupts::restore(primask);

  return count;
}

std::uint32_t hardware_rx_overruns() {
  const auto primask = mcu::interrupts::save_and_disable();

  const auto count = hardware_overruns;

  mcu::interrupts::restore(primask);

  return count;
}

} // namespace drivers::usart2

extern "C" void USART2_IRQHandler() {
  drivers::usart2::handle_rx_interrupt();
  drivers::usart2::handle_tx_interrupt();
}
