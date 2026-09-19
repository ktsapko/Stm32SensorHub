#pragma once

#include "mcu/register.hpp"

#include <cstdint>

namespace mcu::gpio {

constexpr std::uintptr_t gpioa = 0x40020000U;
constexpr std::uintptr_t gpiob = 0x40020400U;

namespace offset {

constexpr std::uintptr_t moder = 0x00U;
constexpr std::uintptr_t otyper = 0x04U;
constexpr std::uintptr_t ospeedr = 0x08U;
constexpr std::uintptr_t pupdr = 0x0CU;
constexpr std::uintptr_t bsrr = 0x18U;
constexpr std::uintptr_t afrl = 0x20U;
constexpr std::uintptr_t afrh = 0x24U;
constexpr std::uintptr_t idr = 0x10U;

} // namespace offset

enum class Mode : std::uint32_t {
  input = 0b00U,
  output = 0b01U,
  alternate = 0b10U,
  analog = 0b11U
};

enum class Speed : std::uint32_t {
  low = 0b00U,
  medium = 0b01U,
  fast = 0b10U,
  very_fast = 0b11U
};

enum class Pull : std::uint32_t { none = 0b00U, up = 0b01U, down = 0b10U };

inline void set_mode(const std::uintptr_t port, const std::uint32_t pin,
                     const Mode mode) {
  const auto shift = pin * 2U;
  const auto mask = 0b11U << shift;
  const auto value = static_cast<std::uint32_t>(mode) << shift;

  modify(port + offset::moder, mask, value);
}

inline void set_open_drain(const std::uintptr_t port, const std::uint32_t pin) {
  set_bits(port + offset::otyper, 1U << pin);
}

inline void set_speed(const std::uintptr_t port, const std::uint32_t pin,
                      const Speed speed) {
  const auto shift = pin * 2U;
  const auto mask = 0b11U << shift;
  const auto value = static_cast<std::uint32_t>(speed) << shift;

  modify(port + offset::ospeedr, mask, value);
}

inline void set_pull(const std::uintptr_t port, const std::uint32_t pin,
                     const Pull pull) {
  const auto shift = pin * 2U;
  const auto mask = 0b11U << shift;
  const auto value = static_cast<std::uint32_t>(pull) << shift;

  modify(port + offset::pupdr, mask, value);
}

inline void set_alternate_function(const std::uintptr_t port,
                                   const std::uint32_t pin,
                                   const std::uint32_t alternate_function) {
  const auto register_offset = pin < 8U ? offset::afrl : offset::afrh;

  const auto shift = (pin % 8U) * 4U;
  const auto mask = 0b1111U << shift;
  const auto value = alternate_function << shift;

  modify(port + register_offset, mask, value);
}

inline void set_output(const std::uintptr_t port, const std::uint32_t pin,
                       const bool enabled) {
  const auto shift = enabled ? pin : pin + 16U;
  reg(port + offset::bsrr) = 1U << shift;
}

inline bool read_input(const std::uintptr_t port, const std::uint32_t pin) {
  return (reg(port + offset::idr) & (1U << pin)) != 0U;
}

} // namespace mcu::gpio