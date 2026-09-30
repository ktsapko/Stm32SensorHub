
#include "drivers/usart2.hpp"

#include "mcu/dma1.hpp"
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

constexpr std::size_t dma_tx_capacity = 64U;
std::uint8_t dma_tx_buffer[dma_tx_capacity]{};

std::uint32_t dropped_bytes = 0U;
std::uint32_t dropped_rx = 0U;
std::uint32_t hardware_overruns = 0U;

std::size_t dma_tx_active_length = 0U;

utils::RingBuffer<tx_buffer_capacity> tx_buffer;
utils::RingBuffer<rx_buffer_capacity> rx_buffer;

enum class DmaTxState { idle, busy, error };
volatile DmaTxState dma_tx_state = DmaTxState::idle;

std::size_t fill_dma_tx_buffer() {
  std::size_t count = 0U;
  while (count < dma_tx_capacity) {
    if (!tx_buffer.pop(dma_tx_buffer[count])) {
      break;
    }
    count++;
  }
  return count;
}

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

void disable_tx_interrupt() {
  mcu::clear_bits(mcu::usart2::cr1,
                  mcu::usart2::cr1_bit::transmit_data_register_empty_interrupt);
}

void enable_rx_interrupt() {
  mcu::set_bits(
      mcu::usart2::cr1,
      mcu::usart2::cr1_bit::receive_data_register_not_empty_interrupt);
}

void clear_dma_tx_flags() {
  mcu::reg(mcu::dma1::hifcr) =
      mcu::dma1::hifcr_bit::cdmeif6 | mcu::dma1::hifcr_bit::cfeif6 |
      mcu::dma1::hifcr_bit::chtif6 | mcu::dma1::hifcr_bit::ctcif6 |
      mcu::dma1::hifcr_bit::cteif6;
}

bool disable_dma_tx_stream() {
  constexpr std::size_t retries = 10U;
  mcu::clear_bits(mcu::dma1::s6cr, mcu::dma1::cr_bit::enable);

  for (std::size_t i = 0; i < retries; i++) {
    if ((mcu::reg(mcu::dma1::s6cr) & mcu::dma1::cr_bit::enable) == 0U) {
      return true;
    }
  }
  return false;
}

bool configure_dma_tx(const std::uint8_t *data, std::size_t length) {

  if (data == nullptr || length == 0U || length > 65535U) {
    return false;
  }
  if (!disable_dma_tx_stream()) {
    return false;
  }
  clear_dma_tx_flags();
  mcu::reg(mcu::dma1::s6cr) = mcu::dma1::cr_field::channel_4 |
                              mcu::dma1::cr_field::memory_to_peripheral |
                              mcu::dma1::cr_bit::memory_increment |
                              mcu::dma1::cr_bit::transfer_complete_interrupt |
                              mcu::dma1::cr_bit::transfer_error_interrupt |
                              mcu::dma1::cr_bit::direct_mode_error_interrupt;
  mcu::reg(mcu::dma1::s6fcr) = mcu::dma1::fcr_bit::fifo_error_interrupt;
  mcu::reg(mcu::dma1::s6par) = mcu::usart2::dr;
  mcu::reg(mcu::dma1::s6m0ar) = reinterpret_cast<std::uintptr_t>(data);
  mcu::reg(mcu::dma1::s6ndtr) = length;
  return true;
}

bool start_dma_tx(const std::uint8_t *data, std::size_t length) {
  if (!configure_dma_tx(data, length)) {
    return false;
  }
  disable_tx_interrupt();
  dma_tx_state = DmaTxState::busy;
  mcu::set_bits(mcu::dma1::s6cr, mcu::dma1::cr_bit::enable);
  mcu::set_bits(mcu::usart2::cr3, mcu::usart2::cr3_bit::dma_transmitter_enable);
  return true;
}

void start_next_dma_tx() {
  if (dma_tx_state != DmaTxState::idle) {
    return;
  }
  const std::size_t length = fill_dma_tx_buffer();
  if (length == 0U) {
    return;
  }
  dma_tx_active_length = length;
  if (!start_dma_tx(dma_tx_buffer, length)) {
    dropped_bytes += length;
    dma_tx_active_length = 0U;
    dma_tx_state = DmaTxState::error;
  }
}

bool stop_dma_tx() {
  mcu::clear_bits(mcu::usart2::cr3,
                  mcu::usart2::cr3_bit::dma_transmitter_enable);
  if (!disable_dma_tx_stream()) {
    return false;
  }
  clear_dma_tx_flags();
  return true;
}

void handle_dma_tx_interrupt() {
  bool error = false;
  bool completed = false;
  const auto status = mcu::reg(mcu::dma1::hisr);
  if (status & (mcu::dma1::hisr_bit::teif6 | mcu::dma1::hisr_bit::dmeif6 |
                mcu::dma1::hisr_bit::feif6)) {
    error = true;

  } else if ((status & mcu::dma1::hisr_bit::tcif6) != 0U) {
    completed = true;
  } else if (!error && !completed) {
    return;
  }
  if (!stop_dma_tx()) {
    dma_tx_state = DmaTxState::error;
    return;
  }
  if (error) {
    dma_tx_active_length = 0U;
    dma_tx_state = DmaTxState::error;
    return;
  }
  dma_tx_active_length = 0U;
  dma_tx_state = DmaTxState::idle;
  start_next_dma_tx();
}

} // namespace

namespace drivers::usart2 {

bool initialize() {
  mcu::rcc::enable_ahb1(mcu::rcc::ahb1::gpioa);
  mcu::rcc::enable_apb1(mcu::rcc::apb1::usart2);
  mcu::rcc::enable_ahb1(mcu::rcc::ahb1::dma1);
  clear_dma_tx_flags();

  configure_tx_pin();
  configure_rx_pin();

  mcu::reg(mcu::usart2::brr) = baud_rate_register;

  mcu::reg(mcu::usart2::cr1) = mcu::usart2::cr1_bit::usart_enable |
                               mcu::usart2::cr1_bit::transmitter_enable |
                               mcu::usart2::cr1_bit::receiver_enable;
  if (!stop_dma_tx()) {
    return false;
  }
  dma_tx_active_length = 0U;
  dma_tx_state = DmaTxState::idle;
  mcu::nvic::enable_irq(mcu::nvic::dma1_stream6_irq);

  enable_rx_interrupt();
  mcu::nvic::enable_irq(mcu::nvic::usart2_irq);
  return true;
}

bool write_byte(const char byte) {
  const auto primask = mcu::interrupts::save_and_disable();

  const bool accepted = tx_buffer.push(static_cast<std::uint8_t>(byte));

  if (accepted) {
    start_next_dma_tx();
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
    start_next_dma_tx();
  }

  mcu::interrupts::restore(primask);

  return accepted;
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

extern "C" void USART2_IRQHandler() { drivers::usart2::handle_rx_interrupt(); }

extern "C" void DMA1_Stream6_IRQHandler() { handle_dma_tx_interrupt(); }
