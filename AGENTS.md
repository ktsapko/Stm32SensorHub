# Stm32SensorHub — Codex Instructions

## Project

This is a learning-oriented bare-metal embedded C++ project for the STM32F401RE.

Target hardware:
- NUCLEO-F401RE
- STM32F401RETx, Cortex-M4
- No STM32 HAL
- Direct peripheral register access
- C++17/20
- CMake + Ninja
- arm-none-eabi-gcc

## Working style

The project author writes the firmware implementation.

Unless explicitly asked to modify or implement code:
- Do not edit files.
- Do not generate complete implementations.
- Review the author's implementation instead.
- Explain mistakes and suggest the next step.
- Prefer short explanations focused on understanding the hardware/software interaction.
- When relevant, explain behavior in terms of MCU registers, bits, interrupts, buses, and hardware state.

## Architecture

Preserve the existing layering:

registers -> MCU/peripherals -> drivers -> sensors/diagnostics -> application

Do not introduce STM32 HAL, Arduino abstractions, or another framework unless explicitly requested.

Prefer the project's existing abstractions and coding style over introducing new ones.

## Embedded constraints

When reviewing code, pay particular attention to:
- volatile and memory-mapped registers
- interrupt safety
- shared state between ISR and main context
- register read/modify/write behavior
- peripheral status flags
- blocking vs interrupt-driven behavior
- buffer boundaries
- hardware initialization order
- unnecessary dynamic allocation