#include "system/freertos.hpp"

#include "FreeRTOS.h"
#include "task.h"

namespace system_rtos {

struct Diagnostics {
  volatile Failure failure;
  TaskHandle_t volatile task;
  const char * volatile task_name;
  const char * volatile file;
  volatile int line;
};

Diagnostics diagnostics{};

[[noreturn]] void halt(const Failure reason) {
  asm volatile("cpsid i" ::: "memory");
  diagnostics.failure = reason;
  for (;;) {
    asm volatile("nop");
  }
}

} // namespace system_rtos

extern "C" void vApplicationGetIdleTaskMemory(
    StaticTask_t **task_buffer,
    StackType_t **stack_buffer,
    uint32_t *stack_size) {
  static StaticTask_t idle_task_buffer;
  static StackType_t idle_stack[configMINIMAL_STACK_SIZE];

  *task_buffer = &idle_task_buffer;
  *stack_buffer = idle_stack;
  *stack_size = configMINIMAL_STACK_SIZE;
}

extern "C" void vApplicationStackOverflowHook(
    TaskHandle_t task, char *task_name) {
  system_rtos::diagnostics.task = task;
  system_rtos::diagnostics.task_name = task_name;
  system_rtos::halt(system_rtos::Failure::stack_overflow);
}

extern "C" void vApplicationMallocFailedHook() {
  system_rtos::halt(system_rtos::Failure::allocation_failed);
}

extern "C" void freertos_assert_failed(
    const char *file, int line) {
  system_rtos::diagnostics.file = file;
  system_rtos::diagnostics.line = line;
  system_rtos::halt(system_rtos::Failure::assertion);
}
