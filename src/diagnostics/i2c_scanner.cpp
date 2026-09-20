#include "diagnostics/i2c_scanner.hpp"

#include "drivers/i2c1.hpp"
#include "drivers/usart2.hpp"

#include <cstdint>

namespace diagnostics::i2c_scanner {

namespace {

constexpr std::uint8_t first_address = 0x08U;
constexpr std::uint8_t last_address = 0x77U;

void write_hex_byte(const std::uint8_t value) {
  constexpr char digits[] = "0123456789ABCDEF";

  drivers::usart2::write("0x");
  drivers::usart2::write_byte(digits[(value >> 4U) & 0x0FU]);
  drivers::usart2::write_byte(digits[value & 0x0FU]);
}

} // namespace

void scan() {
  drivers::usart2::write("Scanning I2C bus...\r\n");

  for (std::uint8_t address = first_address;
       address <= last_address;
       ++address) {

    const auto result = drivers::i2c1::probe(address);

    if (result == drivers::i2c1::ProbeResult::acknowledged) {
      drivers::usart2::write("I2C device found at ");
      write_hex_byte(address);
      drivers::usart2::write("\r\n");
    }
  }

  drivers::usart2::write("I2C scan completed\r\n");
}

} // namespace diagnostics::i2c_scanner
