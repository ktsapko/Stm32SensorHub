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
- Cortex-M4 floating-point unit initialization
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
- Multi-byte I2C register reads
- Dedicated STM32F4 receive sequences for one, two, and multiple bytes
- Generic single-byte I2C register writes
- BMP280 chip ID verification (`0x58`)
- MPU-6050 identity verification (`0x68`)
- Separate BMP280 and MPU-6050 sensor drivers
- BMP280 normal-mode initialization
- BMP280 configuration read-back verification
- 24-byte BMP280 factory-calibration read
- BMP280 little-endian calibration decoding
- Six-byte BMP280 raw measurement read
- BMP280 20-bit pressure and temperature decoding
- Bosch temperature and pressure compensation formulas
- BMP280 pressure output in hectopascals
- MPU-6050 wake-up from sleep mode
- MPU-6050 power-state verification
- Two-byte MPU-6050 axis reads
- Six-byte MPU-6050 accelerometer reads
- Fourteen-byte MPU-6050 measurement reads
- Raw accelerometer, temperature, and gyroscope decoding
- Conversion of MPU-6050 readings into physical units

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
4. Read and verify the BMP280 chip ID.
5. Initialize the BMP280.
6. Read compensated BMP280 temperature and pressure.
7. Read and verify the MPU-6050 identity.
8. Wake the MPU-6050 from sleep mode.
9. Verify the MPU-6050 power state.
10. Read a complete MPU-6050 measurement frame.
11. Report all physical measurements through USART2.
12. Blink the onboard LED continuously.

The application layer does not contain raw MCU peripheral addresses,
sensor addresses, sensor-register addresses, raw sensor values, or
sensor-calibration formulas.

### Sensor-driver layer

The sensor-driver layer contains device-specific behavior:

- `drivers/bmp280` owns the BMP280 address, register map, initialization,
  calibration-data decoding, raw measurement decoding, and compensation
  formulas.
- `drivers/mpu6050` owns the MPU-6050 address, register map, initialization,
  raw-data decoding, and conversion formulas.

Both drivers provide high-level measurement structures containing physical
values. The application does not need to know how the corresponding raw
registers are organized.

### Peripheral-driver layer

The peripheral-driver layer implements MCU peripheral behavior:

- `drivers/usart2` initializes USART2 and transmits text.
- `drivers/i2c1` initializes I2C1, probes device addresses, performs
  single-byte and multi-byte register reads, performs single-byte register
  writes, and reports timeout or acknowledgement errors.

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

### Register read

A register read first sends the internal sensor-register address and then
changes the transfer direction using a repeated START:

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

The public I2C API supports reading one or more consecutive registers:

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
Only the first register address is transmitted by the controller.

### STM32F4 receive sequences

The STM32F4 I2C peripheral requires different receive sequences depending
on the number of requested bytes.

#### One byte

```text
Disable ACK
→ clear ADDR
→ generate STOP
→ wait for RXNE
→ read DR
```

The multi-byte API delegates a one-byte request to the already verified
single-byte implementation.

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

These dedicated sequences ensure that the controller sends NACK and STOP
at the correct time without receiving an unwanted extra byte.

All I2C operations use bounded polling loops to prevent the firmware from
waiting forever if the bus or a sensor does not respond.

### Register write

A register write sends the internal register address followed by its new
value:

```text
START
→ device address + write
→ register address
→ value
→ STOP
```

## Sensor identification and measurements

### BMP280

| Property | Value |
|---|---:|
| I2C address | `0x76` |
| Chip ID register | `0xD0` |
| Expected chip ID | `0x58` |
| Control register | `0xF4` |
| Control value | `0x27` |
| Calibration range | `0x88–0x9F` |
| Calibration size | 24 bytes |
| Measurement range | `0xF7–0xFC` |
| Measurement size | 6 bytes |

#### Initialization

The BMP280 is configured by writing `0x27` to the `CTRL_MEAS` register:

| Field | Value | Configuration |
|---|---:|---|
| `osrs_t` | `001` | Temperature oversampling ×1 |
| `osrs_p` | `001` | Pressure oversampling ×1 |
| `mode` | `11` | Normal mode |

The driver reads `CTRL_MEAS` back after writing it. Initialization fails
if the register value does not match the requested configuration.

#### Factory calibration

Every BMP280 contains factory-programmed calibration coefficients:

```text
dig_T1, dig_T2, dig_T3
dig_P1, dig_P2, dig_P3, dig_P4, dig_P5
dig_P6, dig_P7, dig_P8, dig_P9
```

The 12 coefficients occupy 24 consecutive bytes from `0x88` through
`0x9F`.

BMP280 calibration words are stored in little-endian order:

```text
low byte → high byte
```

`dig_T1` and `dig_P1` are unsigned 16-bit values. The remaining
coefficients are signed 16-bit two's-complement values.

The calibration data are read once during driver initialization and kept
in RAM for subsequent measurement compensation.

#### Raw measurements

Pressure and temperature are read using one six-byte transaction:

| Register range | Measurement |
|---|---|
| `0xF7–0xF9` | Pressure |
| `0xFA–0xFC` | Temperature |

Each raw measurement occupies 20 bits:

```text
MSB[7:0] + LSB[7:0] + XLSB[7:4]
```

The value is assembled as:

```cpp
(msb << 12U) | (lsb << 4U) | (xlsb >> 4U)
```

The lower four bits of `XLSB` are not part of the measurement.

#### Compensation

Raw BMP280 values cannot be used directly as temperature or pressure.
The driver applies the Bosch floating-point compensation formulas using
the factory calibration coefficients.

Temperature compensation first calculates the internal `t_fine` value:

```text
raw temperature + dig_T1–dig_T3 → t_fine → temperature [°C]
```

Pressure compensation uses the same `t_fine` value:

```text
raw pressure + t_fine + dig_P1–dig_P9 → pressure [Pa]
```

The driver converts pressure from pascals to hectopascals:

```text
pressure [hPa] = pressure [Pa] / 100
```

The high-level API returns only physical values:

```cpp
struct Measurements {
  float temperature_c;
  float pressure_hpa;
};

bool read_measurements(Measurements &measurements);
```

Calibration data, raw measurements, `t_fine`, and compensation details
remain inside the BMP280 driver.

### MPU-6050

| Property | Value |
|---|---:|
| I2C address | `0x68` |
| `WHO_AM_I` register | `0x75` |
| Expected identity | `0x68` |
| `PWR_MGMT_1` register | `0x6B` |
| Measurement start register | `0x3B` |
| Measurement frame size | 14 bytes |
| Sleep bit | Bit 6 |

The MPU-6050 starts in sleep mode. The driver writes `0x00` to
`PWR_MGMT_1` and reads the register back to verify that the sleep bit
has been cleared.

A complete measurement frame is read using one 14-byte I2C transaction:

| Register range | Bytes | Measurement |
|---|---:|---|
| `0x3B–0x40` | 6 | Accelerometer X, Y, Z |
| `0x41–0x42` | 2 | Temperature |
| `0x43–0x48` | 6 | Gyroscope X, Y, Z |

Each measurement is stored as a signed 16-bit two's-complement value with
the high byte transmitted first.

#### MPU-6050 conversion formulas

The current configuration uses the default full-scale ranges:

- accelerometer: ±2 g;
- gyroscope: ±250 degrees per second.

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

The temperature value represents the internal sensor temperature and
should not be treated as a precise ambient-air measurement.

Non-zero gyroscope values while stationary are expected because the
sensor has a zero-rate offset. Offset calibration is planned as a future
improvement.

## Floating-point support

The project is compiled for the STM32F401 hardware floating-point unit.

Before entering `main()`, the reset handler enables access to Cortex-M4
coprocessors CP10 and CP11 through the System Control Block `CPACR`
register. The initialization is followed by `DSB` and `ISB` instructions
to ensure that the new access permissions take effect before any
floating-point instruction executes.

Without this initialization, executing a floating-point instruction
causes a `NOCP` UsageFault, which escalates to HardFault.

## Verified serial output

```text
Stm32SensorHub started
BMP280 chip ID = 0x58
BMP280 initialized successfully
BMP280 temperature = 24.66 C
BMP280 pressure = 999.60 hPa
MPU-6050 identity = 0x68
MPU-6050 is awake
Acceleration: X=-0.04 g, Y=-0.02 g, Z=-0.91 g
Temperature: 25.61 C
Angular velocity: X=-5.15 deg/s, Y=2.11 deg/s, Z=-0.18 deg/s
```

The exact measurement values depend on sensor orientation, movement,
temperature, atmospheric pressure, altitude, and sensor offset.

BMP280 pressure is the absolute pressure at the sensor location. Weather
services may report pressure corrected to sea level, so the values can
differ.

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
- enabling the Cortex-M4 floating-point unit;
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

1. Add periodic sensor sampling instead of one startup measurement.
2. Add explicit BMP280 measurement-ready status handling.
3. Add configurable BMP280 oversampling and filter settings.
4. Calibrate the MPU-6050 gyroscope zero-rate offset.
5. Add sensor error recovery for a stuck I2C bus.
6. Add host-side unit tests for byte decoding and compensation formulas.
7. Calculate altitude from compensated atmospheric pressure.