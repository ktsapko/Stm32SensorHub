# Stm32SensorHub

Bare-metal C++20 firmware for the STM32 NUCLEO-F401RE.

The project explores STM32 peripheral and sensor configuration without HAL,
an RTOS, or an operating system. MCU peripherals are controlled directly
through memory-mapped registers.

## Hardware

- STM32 NUCLEO-F401RE
- STM32F401RE Cortex-M4 microcontroller
- BMP280 pressure and temperature sensor
- MPU-6050 accelerometer and gyroscope
- Breadboard and Dupont wires
- Integrated ST-LINK/V2.1 debugger

## Current milestone

Implemented and verified on physical hardware:

- ARM GCC cross-compilation with CMake and Ninja
- Custom startup code and interrupt vector table
- Custom linker script
- Flash and RAM initialization
- Global C++ constructor initialization
- ST-LINK/SWD flashing through OpenOCD
- GPIO output for the onboard LD2 LED on PA5
- USART2 transmission at 115200 baud
- Bare-metal I2C1 initialization
- I2C address probing
- Timeout and status-register diagnostics
- Structured MCU register-access layer
- Minimal bare-metal system-call stubs
- Separate ELF segments for executable Flash and writable RAM
- Warning-free firmware build
- BMP280 detection at I2C address `0x76`
- MPU-6050 detection at I2C address `0x68`
- Single-byte I2C register reads using a repeated START
- Generic single-byte I2C register writes
- BMP280 chip ID verification (`0x58`)
- MPU-6050 identity verification (`0x68`)
- Separate BMP280 and MPU-6050 sensor drivers
- MPU-6050 wake-up from sleep mode
- MPU-6050 power-state verification

## Hardware connections

Both sensors share the same I2C1 bus.

| Nucleo pin | MCU pin | Signal | BMP280 | MPU-6050 |
|---|---|---|---|---|
| `3V3` | — | Power | `VCC` | `VCC` |
| `GND` | — | Ground | `GND` | `GND` |
| `D15` | `PB8` | I2C1 clock | `SCL` | `SCL` |
| `D14` | `PB9` | I2C1 data | `SDA` | `SDA` |

Address configuration:

| Sensor | Configuration | Address |
|---|---|---:|
| BMP280 | `CSB → 3.3 V`, `SDO → GND` | `0x76` |
| MPU-6050 | `AD0 → GND` | `0x68` |

All sensor signals use 3.3 V logic.

## Architecture

The firmware is divided into five layers:

```text
Application
    src/main.cpp
        ↓
Sensor drivers
    drivers/bmp280
    drivers/mpu6050
        ↓
Peripheral drivers
    drivers/usart2
    drivers/i2c1
        ↓
MCU definitions and operations
    mcu/gpio
    mcu/rcc
    mcu/usart2
    mcu/i2c1
        ↓
Memory-mapped register access
    mcu/register
```

### Application layer

`src/main.cpp` defines the firmware execution flow:

1. Initialize the onboard LED.
2. Initialize USART2.
3. Initialize I2C1.
4. Read the BMP280 chip ID.
5. Read the MPU-6050 identity.
6. Wake the MPU-6050 from sleep mode.
7. Verify the MPU-6050 power state.
8. Report the results through USART2.
9. Blink the onboard LED continuously.

The application layer does not contain raw MCU peripheral addresses,
sensor addresses, or sensor-register addresses.

### Sensor-driver layer

The sensor-driver layer contains device-specific behavior:

- `drivers/bmp280` owns the BMP280 I2C address and chip ID register.
- `drivers/mpu6050` owns the MPU-6050 I2C address, identity register,
  power-management register, sleep bit, and wake-up procedure.

The sensor drivers use the generic I2C1 driver and hide device-specific
register details from the application.

### Peripheral-driver layer

The peripheral-driver layer implements MCU peripheral behavior:

- `drivers/usart2` initializes USART2 and transmits text.
- `drivers/i2c1` initializes I2C1, probes device addresses, performs
  single-byte register reads and writes, and reports timeout or
  acknowledgement errors.

### MCU layer

The MCU layer contains STM32F401-specific register addresses, bit masks,
and reusable low-level operations:

- `mcu/register.hpp` provides volatile access to memory-mapped registers.
- `mcu/rcc.hpp` controls peripheral clocks and resets.
- `mcu/gpio.hpp` configures GPIO modes, output type, speed, pull resistors,
  alternate functions, and output state.
- `mcu/usart2.hpp` defines the USART2 register map and relevant bits.
- `mcu/i2c1.hpp` defines the I2C1 register map and relevant bits.

This separation keeps raw MCU addresses out of peripheral drivers,
sensor drivers, and application code while preserving direct
register-level control.

### Register-access layer

`mcu/register.hpp` provides small reusable operations for memory-mapped
register access:

- reading and writing a volatile 32-bit register;
- setting selected bits;
- clearing selected bits;
- modifying a masked register field.

## I2C transactions

### Single-byte register read

A register read first sends the internal sensor-register address and then
changes the transfer direction using a repeated START:

```text
START
→ device address + write
→ register address
→ repeated START
→ device address + read
→ receive one byte
→ NACK
→ STOP
```

For a one-byte STM32F4 master receive, ACK is disabled before clearing
the `ADDR` flag. The controller then generates STOP and reads the received
byte from the data register.

### Single-byte register write

A register write sends the internal register address followed by its new
value:

```text
START
→ device address + write
→ register address
→ value
→ STOP
```

Both operations use bounded polling loops to prevent the firmware from
waiting forever if the bus or a sensor does not respond.

## Sensor identification and initialization

### BMP280

| Property | Value |
|---|---:|
| I2C address | `0x76` |
| Chip ID register | `0xD0` |
| Expected chip ID | `0x58` |

The BMP280 driver currently reads its chip ID to verify communication.
Measurement configuration and calibration-data processing are planned
for the next milestones.

### MPU-6050

| Property | Value |
|---|---:|
| I2C address | `0x68` |
| `WHO_AM_I` register | `0x75` |
| Expected identity | `0x68` |
| `PWR_MGMT_1` register | `0x6B` |
| Sleep bit | Bit 6 |

The MPU-6050 starts in sleep mode. The driver writes `0x00` to
`PWR_MGMT_1` and reads the register back to verify that the sleep bit
has been cleared.

Verified serial output:

```text
Stm32SensorHub started
BMP280 chip ID = 0x58
MPU-6050 identity = 0x68
MPU-6050 is awake
```

## Project structure

```text
Stm32SensorHub/
├── cmake/
│   └── arm-none-eabi-toolchain.cmake
├── include/
│   ├── drivers/
│   │   ├── bmp280.hpp
│   │   ├── i2c1.hpp
│   │   ├── mpu6050.hpp
│   │   └── usart2.hpp
│   └── mcu/
│       ├── gpio.hpp
│       ├── i2c1.hpp
│       ├── rcc.hpp
│       ├── register.hpp
│       └── usart2.hpp
├── linker/
│   └── STM32F401RETx_FLASH.ld
├── openocd/
│   └── nucleo-f401re.cfg
├── src/
│   ├── drivers/
│   │   ├── bmp280.cpp
│   │   ├── i2c1.cpp
│   │   ├── mpu6050.cpp
│   │   └── usart2.cpp
│   ├── system/
│   │   └── syscalls.cpp
│   └── main.cpp
├── startup/
│   └── startup_stm32f401xe.S
├── CMakeLists.txt
└── README.md
```

## Important files

### `startup/startup_stm32f401xe.S`

Contains:

- the initial stack pointer;
- the interrupt vector table;
- `Reset_Handler`;
- copying `.data` from Flash to RAM;
- clearing `.bss`;
- calling global C++ constructors;
- entering `main()`.

### `linker/STM32F401RETx_FLASH.ld`

Defines:

- 512 KiB of Flash starting at `0x08000000`;
- 96 KiB of RAM starting at `0x20000000`;
- the stack-top address;
- placement of the vector table and program sections;
- symbols used by the startup code;
- read-execute permissions for Flash;
- read-write permissions for RAM.

### `src/system/syscalls.cpp`

Provides minimal system-call stubs required by the embedded C/C++ runtime.

The firmware does not use an operating system, filesystem, or host file
descriptors.

### `openocd/nucleo-f401re.cfg`

Contains the OpenOCD configuration for the integrated ST-LINK debugger
and STM32F401RE target.

## Requirements

The current development environment uses:

- Fedora Linux
- CMake 3.28 or newer
- Ninja
- `arm-none-eabi-gcc`
- `arm-none-eabi-g++`
- OpenOCD
- picocom

## Configure

Run once when creating the build directory:

```bash
cmake \
    -S . \
    -B build \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Debug
```

## Build

```bash
cmake --build build
```

Clean rebuild:

```bash
cmake --build build --clean-first
```

Generated firmware images:

```text
build/stm32_sensor_hub.elf
build/stm32_sensor_hub.bin
build/stm32_sensor_hub.hex
```

## Flash

```bash
openocd -f openocd/nucleo-f401re.cfg \
    -c "program build/stm32_sensor_hub.elf verify reset exit"
```

## Serial output

Open the ST-LINK Virtual COM Port:

```bash
picocom --baud 115200 /dev/ttyACM0
```

Exit picocom with:

```text
Ctrl+A, then Ctrl+X
```

## Inspect the firmware

Display firmware size:

```bash
arm-none-eabi-size build/stm32_sensor_hub.elf
```

Inspect ELF memory segments:

```bash
arm-none-eabi-readelf -lW build/stm32_sensor_hub.elf
```

Inspect compiled symbols:

```bash
arm-none-eabi-nm -C build/stm32_sensor_hub.elf
```

Inspect embedded text strings:

```bash
arm-none-eabi-strings build/stm32_sensor_hub.elf
```

## Next steps

1. Add multi-byte I2C register reads.
2. Configure the BMP280 measurement mode.
3. Read BMP280 calibration coefficients.
4. Read raw BMP280 temperature and pressure values.
5. Read raw MPU-6050 accelerometer and gyroscope values.
6. Convert raw sensor values into physical units.
7. Stream measurements through USART2.