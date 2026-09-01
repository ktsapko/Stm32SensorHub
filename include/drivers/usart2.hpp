#pragma once

namespace drivers::usart2 {

void initialize();
void write_byte(char byte);
void write(const char* text);

} // namespace drivers::usart2