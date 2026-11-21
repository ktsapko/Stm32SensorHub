#include "application/sensor_task.hpp"

#include "FreeRTOS.h"
#include "task.h"

#include <cstdint>

namespace {

constexpr TickType_t sampling_period_ticks = pdMS_TO_TICKS(1'000U);
constexpr std::uint32_t sensor_stack_words = 256U;
constexpr UBaseType_t sensor_priority = 1U;
static_assert(sampling_period_ticks > 0U);

StaticTask_t sensor_control;
StackType_t sensor_stack[sensor_stack_words];

void run_sensor_task(void *) {
  TickType_t last_wake = xTaskGetTickCount();

  for (;;) {
    vTaskDelayUntil(&last_wake, sampling_period_ticks);
  }
}

} // namespace

namespace application::sensor_task {

bool create() {
  return xTaskCreateStatic(run_sensor_task, "sensor", sensor_stack_words, nullptr,
                           sensor_priority, sensor_stack,
                           &sensor_control) != nullptr;
}

} // namespace application::sensor_task
