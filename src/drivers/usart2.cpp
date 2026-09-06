#include "drivers/usart2.hpp"

#include "mcu/gpio.hpp"
#include "mcu/rcc.hpp"
#include "mcu/register.hpp"
#include "mcu/usart2.hpp"

#include <cstdint>

namespace {

constexpr std::uint32_t tx_pin = 2U;
constexpr std::uint32_t usart_alternate_function = 7U;

/*
 * Після reset STM32F401 працює від HSI 16 MHz.
 *
 * USARTDIV = 16'000'000 / 115'200 ≈ 138.89
 * BRR = 139 = 0x008B
 */
constexpr std::uint32_t baud_rate_register = 0x008BU;

void configure_tx_pin() {
  mcu::gpio::set_alternate_function(mcu::gpio::gpioa, tx_pin,
                                    usart_alternate_function);

  mcu::gpio::set_mode(mcu::gpio::gpioa, tx_pin, mcu::gpio::Mode::alternate);
}

} // namespace

namespace drivers::usart2 {

void initialize() {
  mcu::rcc::enable_ahb1(mcu::rcc::ahb1::gpioa);
  mcu::rcc::enable_apb1(mcu::rcc::apb1::usart2);

  configure_tx_pin();

  mcu::reg(mcu::usart2::brr) = baud_rate_register;

  mcu::reg(mcu::usart2::cr1) = mcu::usart2::cr1_bit::usart_enable |
                               mcu::usart2::cr1_bit::transmitter_enable;
}

void write_byte(const char byte) {
  auto &status = mcu::reg(mcu::usart2::sr);

  while ((status & mcu::usart2::sr_bit::transmit_data_register_empty) == 0U) {
    // Wait until USART2 can accept another byte.
  }

  mcu::reg(mcu::usart2::dr) =
      static_cast<std::uint32_t>(static_cast<unsigned char>(byte));
}

void write(const char *text) {
  while (*text != '\0') {
    write_byte(*text);
    ++text;
  }
}

} // namespace drivers::usart2