# Stm32SensorHub

Bare-metal C++20 firmware for the STM32 NUCLEO-F401RE.

The project demonstrates direct STM32 peripheral control and communication
with environmental and motion sensors without HAL, an RTOS, or an operating
system. MCU peripherals are controlled through memory-mapped registers.

## Hardware

* STM32 NUCLEO-F401RE
* STM32F401RE Cortex-M4 microcontroller
* BMP280 pressure and temperature sensor
* MPU-6050 accelerometer and gyroscope
* Breadboard and Dupont wires
* Integrated ST-LINK/V2.1 debugger

## Current milestone

Implemented and verified on physical hardware:

* ARM GCC cross-compilation with CMake and Ninja
* Custom startup code and interrupt vector table
* Custom linker script
* Flash and RAM initialization
* Global C++ constructor initialization
* Cortex-M4 floating-point unit initialization
* ST-LINK/SWD flashing through OpenOCD
* GPIO output for the onboard LD2 LED on PA5
* USART2 transmission at 115200 baud
* Bare-metal I2C1 initialization
* I2C address probing
* Single-byte and multi-byte I2C register reads
* Single-byte I2C register writes
* Dedicated STM32F4 receive sequences for one, two, and multiple bytes
* Timeout and status-register diagnostics
* Structured MCU register-access layer
* Minimal bare-metal system-call stubs
* Separate ELF segments for executable Flash and writable RAM
* Warning-free firmware build
* BMP280 detection at I2C address `0x76`
* MPU-6050 detection at I2C address `0x68`
* BMP280 chip ID verification
* BMP280 measurement-mode initialization
* BMP280 factory-calibration reading and decoding
* BMP280 raw pressure and temperature reading
* BMP280 temperature and pressure compensation
* MPU-6050 identity verification
* MPU-6050 wake-up and power-state verification
* MPU-6050 accelerometer, temperature, and gyroscope reading
* Conversion of raw sensor data into physical units
* MPU-6050 gyroscope zero-rate offset calibration
* Runtime gyroscope bias compensation
* Periodic sensor sampling
* SysTick-based millisecond timekeeping
* Interrupt-driven one-second measurement scheduling
* Cortex-M4 `WFI` sleep between SysTick interrupts
* LED heartbeat synchronized with sensor samples
* Shared hardware-independent sensor byte-decoding utilities
* Separate hardware-independent BMP280 compensation implementation
* Native host-side C++ test build with GoogleTest and CTest
* Unit tests for little-endian, big-endian, and 20-bit sensor decoding
* BMP280 temperature and pressure compensation tests using reference values

## Hardware connections

Both sensors share the same I2C1 bus.

| Nucleo pin | MCU pin | Signal     | BMP280 | MPU-6050 |
| ---------- | ------- | ---------- | ------ | -------- |
| `3V3`      | —       | Power      | `VCC`  | `VCC`    |
| `GND`      | —       | Ground     | `GND`  | `GND`    |
| `D15`      | `PB8`   | I2C1 clock | `SCL`  | `SCL`    |
| `D14`      | `PB9`   | I2C1 data  | `SDA`  | `SDA`    |

Address configuration:

| Sensor   | Configuration               | Address |
| -------- | --------------------------- | ------: |
| BMP280   | `CSB → 3.3 V`, `SDO → GND` |  `0x76` |
| MPU-6050 | `AD0 → GND`                 |  `0x68` |

All sensor signals use 3.3 V logic.

## Architecture

The firmware separates application logic, sensor-specific behavior,
hardware-independent sensor algorithms, peripheral drivers, MCU definitions,
and memory-mapped register access.

```text
Application
    src/main.cpp
        ↓
Sensor drivers
    drivers/bmp280
    drivers/mpu6050
        ↓
Sensor algorithms
    sensors/decoding
    sensors/bmp280_compensation
        ↓
Peripheral and system drivers
    drivers/usart2
    drivers/i2c1
    drivers/systick
        ↓
MCU definitions
    mcu/gpio
    mcu/rcc
    mcu/usart2
    mcu/i2c1
    mcu/systick
        ↓
Memory-mapped register access
    mcu/register
```

Hardware-independent sensor algorithms can also be compiled and tested
natively on the development host without STM32 hardware.

### Application layer

`src/main.cpp` defines the firmware execution flow:

1. Initialize the onboard LED.
2. Initialize USART2.
3. Initialize I2C1.
4. Initialize SysTick.
5. Wait for the sensors to complete power-on.
6. Read and verify the BMP280 chip ID.
7. Initialize the BMP280 and load its factory calibration.
8. Read and verify the MPU-6050 identity.
9. Wake the MPU-6050 from sleep mode.
10. Calibrate the MPU-6050 gyroscope zero-rate offset.
11. Enter the periodic sampling loop.
12. Read both sensors once per second.
13. Report measurements through USART2.
14. Toggle the onboard LED after every sample.
15. Sleep between SysTick interrupts using `WFI`.

The application layer does not contain raw peripheral addresses,
sensor-register addresses, raw measurement decoding, or compensation
formulas.

### Sensor-driver layer

The sensor-driver layer contains device-specific communication and state.

#### BMP280 driver

The BMP280 driver owns:

* the I2C device address;
* the chip ID register;
* the measurement configuration;
* the factory-calibration register range;
* the raw measurement register range;
* calibration loading and cached calibration state;
* high-level measurement acquisition;
* conversion of compensated pressure from Pa to hPa.

The driver uses shared decoding utilities for calibration and raw measurement
data and delegates compensation calculations to hardware-independent sensor
logic.

#### MPU-6050 driver

The MPU-6050 driver owns:

* the I2C device address;
* the identity register;
* the power-management register;
* the wake-up procedure;
* the measurement register range;
* acceleration conversion;
* temperature conversion;
* angular-velocity conversion;
* gyroscope zero-rate calibration;
* runtime gyroscope bias compensation.

Signed 16-bit sensor words are decoded using the shared hardware-independent
decoding utilities.

Both sensor drivers expose high-level measurement structures containing
physical values.

### Sensor-algorithm layer

The sensor-algorithm layer contains logic that does not access STM32
peripherals or sensor buses directly.

#### Sensor decoding

`include/sensors/decoding.hpp` provides `constexpr` helpers for:

* unsigned 16-bit little-endian decoding;
* signed 16-bit little-endian decoding;
* signed 16-bit big-endian decoding;
* unsigned 20-bit BMP280 measurement decoding.

The BMP280 and MPU-6050 drivers use the same decoding implementation instead
of maintaining separate byte-conversion code.

#### BMP280 compensation

`src/sensors/bmp280_compensation.cpp` contains the Bosch floating-point
temperature and pressure compensation calculations.

The implementation depends only on calibration coefficients and raw
measurement values. It does not access I2C or STM32 registers, which allows
the production compensation code to be compiled directly into native
host-side unit tests.

### Peripheral and system-driver layer

* `drivers/usart2` initializes USART2 and transmits text.
* `drivers/i2c1` initializes I2C1 and performs register transactions.
* `drivers/systick` initializes the Cortex-M4 SysTick timer, maintains a
  millisecond counter, and provides millisecond delays.

### MCU layer

The MCU layer contains STM32F401 and Cortex-M4 register addresses and bit
masks:

* `mcu/register.hpp` provides volatile memory-mapped register access.
* `mcu/rcc.hpp` controls clocks and peripheral resets.
* `mcu/gpio.hpp` configures GPIO modes and output state.
* `mcu/usart2.hpp` defines the USART2 register map.
* `mcu/i2c1.hpp` defines the I2C1 register map.
* `mcu/systick.hpp` defines the Cortex-M4 SysTick register map.

## I2C transactions

### Register read

A register read sends the internal sensor-register address and then changes
the transfer direction using a repeated START:

```text
START
→ device address + write
→ register address
→ repeated START
→ device address + read
→ receive data
→ NACK
→ STOP
```

The public I2C API supports one or more consecutive registers:

```cpp
ReadResult read_register(
    std::uint8_t address,
    std::uint8_t register_address,
    std::uint8_t &value);

ReadResult read_registers(
    std::uint8_t address,
    std::uint8_t start_register,
    std::uint8_t *buffer,
    std::size_t length);
```

Multi-byte reads use the sensor's automatic register-address increment.

### STM32F4 receive sequences

The STM32F4 I2C peripheral requires different receive sequences depending
on the requested number of bytes.

#### One byte

```text
Disable ACK
→ clear ADDR
→ generate STOP
→ wait for RXNE
→ read DR
```

#### Two bytes

```text
Set POS
→ disable ACK
→ clear ADDR
→ wait for BTF
→ generate STOP
→ read DR twice
→ restore ACK and POS
```

#### More than two bytes

ACK remains enabled while the initial bytes are received. When three bytes
remain:

```text
Wait for BTF
→ disable ACK
→ read byte N-2
→ wait for BTF
→ generate STOP
→ read bytes N-1 and N
→ restore ACK and POS
```

These sequences ensure that NACK and STOP are generated at the correct time
without receiving an unwanted extra byte.

All I2C operations use bounded polling loops to prevent the firmware from
waiting forever if the bus or a sensor does not respond.

### Register write

```text
START
→ device address + write
→ register address
→ value
→ STOP
```

## BMP280

### Identification and configuration

| Property             |       Value |
| -------------------- | ----------: |
| I2C address          |      `0x76` |
| Chip ID register     |      `0xD0` |
| Expected chip ID     |      `0x58` |
| `CTRL_MEAS` register |      `0xF4` |
| `CTRL_MEAS` value    |      `0x27` |
| Calibration range    | `0x88–0x9F` |
| Calibration size     |    24 bytes |
| Measurement range    | `0xF7–0xFC` |
| Measurement size     |     6 bytes |

The `CTRL_MEAS` value `0x27` configures:

| Field    | Value | Configuration               |
| -------- | ----: | --------------------------- |
| `osrs_t` | `001` | Temperature oversampling ×1 |
| `osrs_p` | `001` | Pressure oversampling ×1    |
| `mode`   |  `11` | Normal mode                 |

The driver reads `CTRL_MEAS` back after writing it and fails initialization
if the value does not match.

### Factory calibration

The BMP280 contains 12 factory-programmed coefficients:

```text
dig_T1, dig_T2, dig_T3
dig_P1, dig_P2, dig_P3, dig_P4, dig_P5
dig_P6, dig_P7, dig_P8, dig_P9
```

They occupy 24 consecutive bytes from `0x88` through `0x9F`.

Calibration words are stored in little-endian order:

```text
low byte → high byte
```

`dig_T1` and `dig_P1` are unsigned 16-bit values. The remaining
coefficients are signed 16-bit two's-complement values.

Calibration data are read once during initialization and cached in RAM.

### Raw measurements

Pressure and temperature are read in one six-byte I2C transaction:

| Register range | Measurement |
| -------------- | ----------- |
| `0xF7–0xF9`    | Pressure    |
| `0xFA–0xFC`    | Temperature |

Each raw measurement occupies 20 bits:

```text
MSB[7:0] + LSB[7:0] + XLSB[7:4]
```

The value is assembled as:

```cpp
(msb << 12U) | (lsb << 4U) | (xlsb >> 4U)
```

The lower four bits of `XLSB` are not part of the measurement.

### Compensation

Raw values cannot be used directly. The firmware applies the Bosch
floating-point compensation formulas.

Temperature processing:

```text
raw temperature
+ dig_T1–dig_T3
→ t_fine
→ temperature [°C]
```

Pressure processing:

```text
raw pressure
+ t_fine
+ dig_P1–dig_P9
→ pressure [Pa]
→ pressure [hPa]
```

The compensation implementation is hardware-independent and is shared by
the embedded firmware and native host-side unit tests.

High-level API:

```cpp
struct Measurements {
  float temperature_c;
  float pressure_hpa;
};

bool read_measurements(Measurements &measurements);
```

BMP280 pressure is the absolute pressure at the sensor location. Weather
services often report pressure corrected to sea level, so those values can
differ.

## MPU-6050

### Identification and initialization

| Property                   |    Value |
| -------------------------- | -------: |
| I2C address                |   `0x68` |
| `WHO_AM_I` register        |   `0x75` |
| Expected identity          |   `0x68` |
| `PWR_MGMT_1` register      |   `0x6B` |
| Measurement start register |   `0x3B` |
| Measurement frame size     | 14 bytes |
| Sleep bit                  |    Bit 6 |

The MPU-6050 starts in sleep mode. The driver writes `0x00` to
`PWR_MGMT_1` and reads it back to verify that the sleep bit is clear.

After wake-up verification, the firmware calibrates the gyroscope zero-rate
offset before periodic sensor sampling begins.

### Measurement frame

A complete measurement frame is read using one 14-byte I2C transaction:

| Register range | Bytes | Measurement           |
| -------------- | ----: | --------------------- |
| `0x3B–0x40`    |     6 | Accelerometer X, Y, Z |
| `0x41–0x42`    |     2 | Temperature           |
| `0x43–0x48`    |     6 | Gyroscope X, Y, Z     |

Each measurement is a signed 16-bit two's-complement value with the high
byte transmitted first.

### Conversion formulas

The current configuration uses the default full-scale ranges:

* accelerometer: ±2 g;
* gyroscope: ±250 degrees per second.

Acceleration:

```text
acceleration [g] = raw acceleration / 16384
```

Temperature:

```text
temperature [°C] = raw temperature / 340 + 36.53
```

Angular velocity:

```text
angular velocity [°/s] = raw gyroscope / 131
```

The temperature represents the internal sensor temperature and is not a
precise ambient-air measurement.

### Gyroscope calibration

The MPU-6050 gyroscope has a zero-rate offset, which can produce non-zero
angular-velocity readings while the sensor is stationary.

During initialization, the firmware performs a startup calibration using
100 uncorrected gyroscope samples while the board is stationary.

For each axis, the driver calculates the average measured angular velocity:

```text
bias = sum of uncorrected samples / sample count
```

The calculated X, Y, and Z bias values are stored in RAM.

Normal high-level measurement reads subtract the corresponding bias from
each gyroscope axis:

```text
corrected angular velocity = measured angular velocity - bias
```

Calibration uses a separate uncorrected measurement path. This prevents an
existing bias value from being applied while a new bias is being calculated.

The board must remain stationary during startup calibration.

After calibration, a stationary sensor reports angular velocity close to
zero while preserving its response to actual rotation.

## SysTick scheduling

### Configuration

The Cortex-M4 SysTick timer generates one interrupt every millisecond.

The current firmware uses the default 16 MHz HSI system clock:

```text
system clock = 16,000,000 Hz
ticks per millisecond = 16,000
reload value = 15,999
```

SysTick uses the processor clock and enables:

* `ENABLE` — start the counter;
* `TICKINT` — generate an exception at zero;
* `CLKSOURCE` — use the processor clock.

If the system clock configuration changes, the SysTick clock constant must
also be updated.

### Interrupt handler

`SysTick_Handler` is registered in the Cortex-M4 vector table.

Every interrupt increments a 32-bit millisecond counter:

```cpp
milliseconds_counter = milliseconds_counter + 1U;
```

The counter wraps after approximately 49.7 days. Time comparisons use
unsigned subtraction:

```cpp
current_time - previous_time
```

This remains correct across counter overflow for intervals shorter than
half of the counter range.

### Periodic sampling

The application checks elapsed system time:

```cpp
if ((current_time - last_sample_time) >= sampling_period_ms) {
  last_sample_time += sampling_period_ms;
  report_sensor_sample(...);
}
```

Using:

```cpp
last_sample_time += sampling_period_ms;
```

keeps samples aligned to the system timeline and prevents measurement and
USART execution time from being added to every sampling period.

Between interrupts, the MCU executes:

```cpp
asm volatile("wfi");
```

`WFI` means Wait For Interrupt. The Cortex-M4 sleeps and wakes when the
next SysTick interrupt occurs.

The current sampling period is:

```text
1000 ms
```

The onboard LED changes state after every sample and acts as a visual
heartbeat.

## Floating-point support

The firmware is compiled for the STM32F401 single-precision hardware
floating-point unit.

Before entering `main()`, `Reset_Handler` enables access to Cortex-M4
coprocessors CP10 and CP11 through `SCB_CPACR`.

The initialization is followed by:

```asm
dsb
isb
```

Without this initialization, executing a floating-point instruction causes
a `NOCP` UsageFault, which escalates to HardFault.

## Host-side unit tests

Hardware-independent sensor logic is tested natively on the Fedora
development host using GoogleTest and CTest.

The host tests use the native C++ compiler rather than the ARM
cross-compiler:

```text
STM32 firmware
    arm-none-eabi-g++
    ↓
    build/

Host-side tests
    native g++
    + GoogleTest
    ↓
    build-tests/
```

The current test suite contains seven tests.

### Sensor decoding tests

The decoding tests verify:

* unsigned 16-bit little-endian decoding;
* signed positive 16-bit little-endian decoding;
* signed negative 16-bit little-endian decoding;
* signed 16-bit big-endian decoding;
* unsigned 20-bit BMP280 measurement decoding.

### BMP280 compensation tests

The compensation tests use reference calibration coefficients and raw ADC
values:

```text
raw temperature = 519888
raw pressure    = 415148
```

Expected compensated values:

```text
temperature ≈ 25.08 °C
pressure    ≈ 100653 Pa
```

The production `bmp280_compensation.cpp` implementation is compiled directly
into the native test executable. The tests therefore verify the same
compensation code that is used by the STM32 firmware.

### Configure and run host tests

Configure the native test build:

```bash
cmake -S tests -B build-tests -G Ninja
```

Build the tests:

```bash
cmake --build build-tests
```

Run the complete test suite:

```bash
ctest --test-dir build-tests --output-on-failure
```

Current result:

```text
100% tests passed, 0 tests failed out of 7
```

The host test build is intentionally separate from the ARM firmware build.

## Verified serial output

```text
Stm32SensorHub started
BMP280 chip ID = 0x58
BMP280 initialized successfully
MPU-6050 identity = 0x68
MPU-6050 is awake
Calibrating MPU-6050 gyroscope...
MPU-6050 gyroscope calibration succeeded

--- Sensor sample ---
BMP280 temperature = 26.85 C
BMP280 pressure = 997.91 hPa
Acceleration: X=-0.04 g, Y=-0.02 g, Z=-0.93 g
Temperature: 27.73 C
Angular velocity: X=0.06 deg/s, Y=0.05 deg/s, Z=-0.13 deg/s
```

A new sample is produced approximately once per second.

When the board is tilted, the gravity vector moves between the
accelerometer axes. During rotation, the gyroscope reports angular
velocity changes.

After the board stops moving, the calibrated gyroscope returns to values
close to zero.

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
│   │   ├── systick.hpp
│   │   └── usart2.hpp
│   ├── mcu/
│   │   ├── gpio.hpp
│   │   ├── i2c1.hpp
│   │   ├── rcc.hpp
│   │   ├── register.hpp
│   │   ├── systick.hpp
│   │   └── usart2.hpp
│   └── sensors/
│       └── decoding.hpp
├── linker/
│   └── STM32F401RETx_FLASH.ld
├── openocd/
│   └── nucleo-f401re.cfg
├── src/
│   ├── drivers/
│   │   ├── bmp280.cpp
│   │   ├── i2c1.cpp
│   │   ├── mpu6050.cpp
│   │   ├── systick.cpp
│   │   └── usart2.cpp
│   ├── sensors/
│   │   └── bmp280_compensation.cpp
│   ├── system/
│   │   └── syscalls.cpp
│   └── main.cpp
├── startup/
│   └── startup_stm32f401xe.S
├── tests/
│   ├── CMakeLists.txt
│   ├── bmp280_compensation_test.cpp
│   └── decoding_test.cpp
├── CMakeLists.txt
└── README.md
```

## Important files

### `startup/startup_stm32f401xe.S`

Contains:

* the initial stack pointer;
* the interrupt vector table;
* `Reset_Handler`;
* the `SysTick_Handler` vector;
* floating-point unit initialization;
* copying `.data` from Flash to RAM;
* clearing `.bss`;
* calling global C++ constructors;
* entering `main()`.

### `linker/STM32F401RETx_FLASH.ld`

Defines:

* 512 KiB of Flash starting at `0x08000000`;
* 96 KiB of RAM starting at `0x20000000`;
* the stack-top address;
* placement of the vector table and program sections;
* symbols used by startup code;
* read-execute permissions for Flash;
* read-write permissions for RAM.

### `include/sensors/decoding.hpp`

Contains hardware-independent `constexpr` functions for converting sensor
byte sequences into integer values.

The same implementation is used by the BMP280 and MPU-6050 drivers and by
native host-side tests.

### `src/sensors/bmp280_compensation.cpp`

Contains the hardware-independent BMP280 floating-point compensation
implementation.

It is compiled into both the STM32 firmware and the native BMP280
compensation test executable.

### `src/system/syscalls.cpp`

Provides minimal system-call stubs required by the embedded C/C++ runtime.

The firmware does not use an operating system, filesystem, or host file
descriptors.

### `tests/`

Contains the native GoogleTest test suite and its independent CMake
configuration.

The tests exercise hardware-independent production code without requiring
the STM32 board or I2C peripherals.

## Requirements

The current development environment uses:

### Firmware

* Fedora Linux
* CMake 3.28 or newer
* Ninja
* `arm-none-eabi-gcc`
* `arm-none-eabi-g++`
* OpenOCD
* picocom

### Host-side tests

* Native C++20 compiler
* GoogleTest
* CTest

## Configure firmware

Run once when creating the firmware build directory:

```bash
cmake \
    -S . \
    -B build \
    -G Ninja \
    -DCMAKE_TOOLCHAIN_FILE=cmake/arm-none-eabi-toolchain.cmake \
    -DCMAKE_BUILD_TYPE=Debug
```

## Build firmware

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

Inspect embedded strings:

```bash
arm-none-eabi-strings build/stm32_sensor_hub.elf
```

## Next steps

1. Add recovery for a stuck I2C bus.
2. Add sensor retry and reinitialization logic.
3. Add configurable BMP280 oversampling and filtering.
4. Calculate altitude from compensated atmospheric pressure.
5. Replace blocking USART transmission with buffered interrupt-driven output.
6. Configure the STM32F401 PLL and derive peripheral clocks explicitly.