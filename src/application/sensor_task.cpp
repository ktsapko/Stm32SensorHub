#include "application/sensor_task.hpp"

#include "application/sensor_hub.hpp"

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
application::sensor_hub::Sample latest_sample{};
volatile std::uint32_t sensor_wake_count = 0U;

void run_sensor_task(void *) {
  TickType_t last_wake = xTaskGetTickCount();

  // Allow the sensors to power on before the first I2C transaction.
  vTaskDelayUntil(&last_wake, sampling_period_ticks);
  const auto ready = application::sensor_hub::initialize_sensors();

  last_wake = xTaskGetTickCount();
  for (;;) {
    vTaskDelayUntil(&last_wake, sampling_period_ticks);
    latest_sample = application::sensor_hub::acquire_sample(ready);
    sensor_wake_count = sensor_wake_count + 1U;
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
