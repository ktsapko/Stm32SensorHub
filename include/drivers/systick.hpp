#pragma once

#include <cstdint>

namespace drivers::systick {

void initialize();
void delay_ms(std::uint32_t milliseconds);

} // namespace drivers::systick