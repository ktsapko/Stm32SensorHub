#include "drivers/bmp280.hpp"

#include "drivers/i2c1.hpp"

#include <cstdint>

namespace drivers::bmp280 {
namespace {

constexpr std::uint8_t address = 0x76U;
constexpr std::uint8_t chip_id_register = 0xD0U;

} // namespace

bool read_chip_id(std::uint8_t &chip_id) {
  return drivers::i2c1::read_register(address, chip_id_register, chip_id) ==
         drivers::i2c1::ReadResult::success;
}
} // namespace drivers::bmp280