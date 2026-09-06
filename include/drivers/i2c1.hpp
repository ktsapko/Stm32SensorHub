#pragma once

#include <cstdint>

namespace drivers::i2c1 {

enum class ProbeResult : std::uint8_t {
    acknowledged,
    bus_busy_timeout,
    start_timeout,
    not_acknowledged,
    response_timeout
};

void initialize();

ProbeResult probe(std::uint8_t address);

std::uint32_t last_sr1();
std::uint32_t last_sr2();

} // namespace drivers::i2c1