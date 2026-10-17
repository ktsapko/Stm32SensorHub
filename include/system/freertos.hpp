#pragma once

namespace system_rtos {

enum class Failure {
  none,
  assertion,
  stack_overflow,
  allocation_failed,
  hardware_initialization,
  task_creation,
  scheduler_returned
};

[[noreturn]] void halt(Failure reason);

} // namespace system_rtos
