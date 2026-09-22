#pragma once

#include "drivers/bmp280.hpp"

namespace graphics::sensor_dashboard {

bool render(const drivers::bmp280::Measurements &measurements);

} // namespace graphics::sensor_dashboard
