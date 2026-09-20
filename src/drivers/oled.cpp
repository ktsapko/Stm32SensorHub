#include "drivers/oled.hpp"

#include "drivers/i2c1.hpp"

#include <cstdint>

namespace drivers::oled {

namespace {

constexpr std::uint8_t device_address = 0x3CU;

constexpr std::uint8_t command_control_byte = 0x00U;

constexpr std::uint8_t display_off = 0xAEU;
constexpr std::uint8_t display_on = 0xAFU;

constexpr std::uint8_t entire_display_on = 0xA5U;
constexpr std::uint8_t resume_ram_display = 0xA4U;

constexpr std::uint8_t set_display_clock = 0xD5U;
constexpr std::uint8_t display_clock_value = 0x80U;

constexpr std::uint8_t set_multiplex_ratio = 0xA8U;
constexpr std::uint8_t multiplex_ratio_64 = 0x3FU;

constexpr std::uint8_t set_display_offset = 0xD3U;
constexpr std::uint8_t display_offset_zero = 0x00U;

constexpr std::uint8_t set_start_line = 0x40U;

constexpr std::uint8_t segment_remap = 0xA1U;
constexpr std::uint8_t scan_direction = 0xC8U;

constexpr std::uint8_t set_contrast = 0x81U;
constexpr std::uint8_t contrast_value = 0x7FU;

constexpr std::uint8_t normal_display = 0xA6U;

constexpr std::uint8_t set_charge_pump = 0xADU;
constexpr std::uint8_t charge_pump_enable = 0x8BU;

constexpr std::size_t display_width = 128U;
constexpr std::size_t display_height = 64U;

constexpr std::size_t page_height = 8U;
constexpr std::size_t page_count = display_height / page_height;

constexpr std::size_t framebuffer_size = display_width * page_count;

constexpr std::uint8_t column_offset = 2U;

constexpr std::size_t transfer_chunk_size = 16U;

constexpr std::uint8_t data_control_byte = 0x40U;

constexpr std::uint8_t set_page_address = 0xB0U;
constexpr std::uint8_t set_lower_column = 0x00U;
constexpr std::uint8_t set_higher_column = 0x10U;

std::uint8_t framebuffer[framebuffer_size]{};

bool write_command(const std::uint8_t command) {
  const std::uint8_t buffer[] = {command_control_byte, command};

  return drivers::i2c1::write(device_address, buffer, sizeof(buffer)) ==
         drivers::i2c::WriteResult::success;
}

bool write_data(const std::uint8_t *const data, const std::size_t length) {
  if (data == nullptr || length == 0U || length > transfer_chunk_size) {
    return false;
  }

  std::uint8_t buffer[transfer_chunk_size + 1U]{};

  buffer[0] = data_control_byte;

  for (std::size_t i = 0U; i < length; ++i) {
    buffer[i + 1U] = data[i];
  }

  return drivers::i2c1::write(device_address, buffer, length + 1U) ==
         drivers::i2c::WriteResult::success;
}

} // namespace

bool initialize() {
  const std::uint8_t commands[] = {display_off,

                                   set_display_clock,   display_clock_value,

                                   set_multiplex_ratio, multiplex_ratio_64,

                                   set_display_offset,  display_offset_zero,

                                   set_start_line,

                                   segment_remap,       scan_direction,

                                   set_contrast,        contrast_value,

                                   normal_display,

                                   set_charge_pump,     charge_pump_enable,

                                   resume_ram_display,

                                   display_on};

  for (const auto command : commands) {
    if (!write_command(command)) {
      return false;
    }
  }

  return true;
}

bool display_all_on() { return write_command(entire_display_on); }

bool display_normal() { return write_command(resume_ram_display); }

void clear() {
  for (auto &byte : framebuffer) {
    byte = 0U;
  }
}

void set_pixel(const std::uint8_t x, const std::uint8_t y) {
  if (x >= display_width || y >= display_height) {
    return;
  }

  const std::size_t index =
      static_cast<std::size_t>(x) +
      (static_cast<std::size_t>(y) / page_height) * display_width;

  const auto bit = static_cast<std::uint8_t>(1U << (y % page_height));

  framebuffer[index] |= bit;
}

bool flush() {
  for (std::size_t page = 0U; page < page_count; ++page) {

    const auto page_address =
        static_cast<std::uint8_t>(set_page_address | page);

    if (!write_command(page_address)) {
      return false;
    }

    if (!write_command(static_cast<std::uint8_t>(set_lower_column |
                                                 (column_offset & 0x0FU)))) {
      return false;
    }

    if (!write_command(static_cast<std::uint8_t>(set_higher_column |
                                                 (column_offset >> 4U)))) {
      return false;
    }

    const std::size_t page_start = page * display_width;

    for (std::size_t offset = 0U; offset < display_width;
         offset += transfer_chunk_size) {

      if (!write_data(&framebuffer[page_start + offset], transfer_chunk_size)) {
        return false;
      }
    }
  }

  return true;
}

} // namespace drivers::oled

