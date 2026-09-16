#pragma once

#include <cstdint>

namespace drivers::systick {

void initialize();

std::uint32_t milliseconds();

void delay_ms(std::uint32_t duration_ms);

} // namespace drivers::systick