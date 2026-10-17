#include "drivers/usart2.hpp"
#include "mcu/gpio.hpp"
#include "mcu/rcc.hpp"
#include "mcu/scb.hpp"
#include "system/freertos.hpp"

#include "FreeRTOS.h"
#include "task.h"

#include <cstdint>

namespace {

constexpr std::uint32_t led_pin = 5U;
constexpr std::uint32_t heartbeat_period_ms = 1'000U;
constexpr std::uint32_t heartbeat_stack_words = 256U;
constexpr UBaseType_t heartbeat_priority = 1U;

StaticTask_t heartbeat_control;
StackType_t heartbeat_stack[heartbeat_stack_words];
volatile UBaseType_t heartbeat_stack_free_words = 0U;

void initialize_led() {
  mcu::rcc::enable_ahb1(mcu::rcc::ahb1::gpioa);
  mcu::gpio::set_output(mcu::gpio::gpioa, led_pin, false);
  mcu::gpio::set_mode(mcu::gpio::gpioa, led_pin, mcu::gpio::Mode::output);
}

void heartbeat_task(void *) {
  TickType_t last_wake = xTaskGetTickCount();
  constexpr TickType_t period = pdMS_TO_TICKS(heartbeat_period_ms);
  static_assert(period > 0U);
  bool enabled = false;

  for (;;) {
    enabled = !enabled;
    mcu::gpio::set_output(mcu::gpio::gpioa, led_pin, enabled);
    heartbeat_stack_free_words = uxTaskGetStackHighWaterMark(nullptr);
    vTaskDelayUntil(&last_wake, period);
  }
}

} // namespace

int main() {
  mcu::scb::use_all_preemption_priority_bits();
  initialize_led();

  if (!drivers::usart2::initialize()) {
    system_rtos::halt(system_rtos::Failure::hardware_initialization);
  }
  drivers::usart2::write("Stm32SensorHub FreeRTOS starting\r\n");

  // Sensor/OLED initialization and console processing migrate in later milestones.
  const TaskHandle_t heartbeat = xTaskCreateStatic(
      heartbeat_task, "heartbeat", heartbeat_stack_words, nullptr,
      heartbeat_priority, heartbeat_stack, &heartbeat_control);
  if (heartbeat == nullptr) {
    system_rtos::halt(system_rtos::Failure::task_creation);
  }

  vTaskStartScheduler();
  system_rtos::halt(system_rtos::Failure::scheduler_returned);
}
