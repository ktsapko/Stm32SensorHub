# Stm32SensorHub

Bare-metal C++ firmware for the STM32 NUCLEO-F401RE.

## Current milestone

- ARM GCC cross-compilation with CMake and Ninja
- Custom linker script and startup code
- ST-LINK/SWD flashing through OpenOCD
- GPIO output controlling the onboard LD2 LED on PA5

## Build

```bash
cmake \
    -S . \
    -B build \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Debug

cmake --build build