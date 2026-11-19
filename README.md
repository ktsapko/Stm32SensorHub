# Stm32SensorHub

Bare-metal C++20 firmware for the STM32 NUCLEO-F401RE.

The project demonstrates direct STM32 peripheral control, environmental and
motion sensor drivers and graphical OLED output without STM32 HAL.
The firmware uses FreeRTOS for task scheduling.

MCU peripherals are controlled through memory-mapped registers.

The firmware includes custom peripheral drivers, hardware-independent sensor
algorithms, native unit tests, I2C bus recovery, and a lightweight graphics
system for an SH1106 OLED display.

## Live Sensor Dashboard

The project includes a sensor dashboard for a 1.30-inch SH1106 OLED display.
Sensor acquisition and dashboard updates are not active in the current FreeRTOS firmware.

![STM32 Sensor Hub running the live BMP280 dashboard](docs/images/oled-sensor-dashboard.jpg)

The dashboard displays:

- BMP280 temperature in degrees Celsius
- Atmospheric pressure in hPa
- Sensor identification
- STM32 Sensor Hub header

The dashboard and sensor drivers are available in the source tree, but are not
initialized or scheduled by the current `main()`.

## Hardware

### Development board

- STM32 NUCLEO-F401RE
- STM32F401RE Cortex-M4 microcontroller
- 512 KiB Flash
- 96 KiB SRAM
- Integrated ST-LINK/V2.1 debugger

### Connected devices

| Device | Function | Interface | Address |
|--------|----------|-----------|---------|
| BMP280 | Temperature and atmospheric pressure | I2C1 | `0x76` |
| MPU-6050 | Acceleration, temperature, angular velocity | I2C1 | `0x68` |
| SH1106 OLED | 128×64 monochrome graphical display | I2C1 | `0x3C` |

Additional hardware:

- Breadboard
- Dupont wires
- USB connection for flashing and serial diagnostics

## Implemented components

The following components are implemented. Sensor acquisition, dashboard updates,
and their application diagnostics are not active in the current runtime:

### Platform and peripherals

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
- Structured MCU register-access layer
- Minimal bare-metal system-call stubs
- Separate ELF segments for executable Flash and writable RAM

### I2C communication

- I2C address probing
- I2C bus scanner
- Single-byte register reads
- Multi-byte register reads
- Single-byte register writes
- Generic multi-byte I2C transmission
- Dedicated STM32F4 receive sequences for one, two, and multiple bytes
- Bounded polling and timeout handling
- Status-register diagnostics
- Automatic recovery from a stuck I2C bus
- GPIO-based recovery using up to nine SCL clock pulses
- Manual STOP generation during bus recovery
- I2C1 peripheral reinitialization after recovery
- Runtime I2C recovery diagnostics
- Physical stuck-SDA fault testing

### Environmental and motion sensors

- BMP280 detection and chip ID verification
- BMP280 measurement-mode initialization
- Factory-calibration reading and decoding
- Temperature and pressure compensation
- MPU-6050 identity verification
- MPU-6050 wake-up and power-state verification
- Accelerometer, temperature, and gyroscope reading
- Conversion of raw measurements into physical units
- Gyroscope zero-rate offset calibration
- Runtime gyroscope bias compensation

### Scheduling

- FreeRTOS preemptive scheduler with a 1 kHz tick
- Statically allocated heartbeat and idle tasks
- One-second LED toggle using `vTaskDelayUntil`

### OLED and graphics

- SH1106 OLED detection at I2C address `0x3C`
- SH1106 initialization and display control
- Generic I2C command and data transmission
- 128×64 monochrome framebuffer
- Individual pixel drawing
- Page-based OLED memory addressing
- SH1106 column-offset handling
- Chunked framebuffer transmission
- Custom 5×7 bitmap font
- Uppercase and lowercase Latin alphabet
- Numeric glyphs and space
- Text rendering with display-boundary checking
- Physical verification of graphical and text output

### Live sensor dashboard

- Custom floating-point number formatting without `sprintf`
- Fixed-point formatting with two decimal places
- Buffer-size validation and invalid-value handling
- 13 native GoogleTest cases for number formatting
- Dedicated sensor dashboard rendering
- Live BMP280 temperature and pressure visualization
- Shared measurements for OLED and USART2 output
- Physical verification on NUCLEO-F401RE

### Architecture and testing

- Shared hardware-independent sensor byte-decoding utilities
- Hardware-independent BMP280 compensation implementation
- Native host-side C++ test build with GoogleTest and CTest
- Unit tests for little-endian, big-endian, and 20-bit sensor decoding
- BMP280 compensation tests using reference values
- Hardware-independent I2C bus abstraction
- Dependency injection of the production STM32 I2C1 implementation
- Host-side mock I2C bus
- BMP280 and MPU-6050 driver tests
- Sensor-driver I2C failure-path testing
- MPU-6050 register-write verification
- Number formatting tests

## Hardware connections

BMP280, MPU-6050, and SH1106 share the same I2C1 bus.

| Nucleo pin | MCU pin | Signal | BMP280 | MPU-6050 | OLED |
|------------|---------|--------|--------|----------|------|
| `3V3` | — | Power | `VCC` | `VCC` | `VDD` |
| `GND` | — | Ground | `GND` | `GND` | `GND` |
| `D15` | `PB8` | I2C1 SCL | `SCL` | `SCL` | `SCK` |
| `D14` | `PB9` | I2C1 SDA | `SDA` | `SDA` | `SDA` |

The OLED module labels its I2C clock pin `SCK`.

All connected devices use 3.3 V logic.

### Address configuration

| Device | Configuration | Address |
|--------|---------------|---------|
| BMP280 | `CSB → 3.3 V`, `SDO → GND` | `0x76` |
| MPU-6050 | `AD0 → GND` | `0x68` |
| SH1106 OLED | Module configuration | `0x3C` |

All three addresses have been verified using the custom I2C scanner.

## Architecture

The firmware separates application logic, sensor-specific behavior,
hardware-independent algorithms, diagnostics, peripheral drivers,
graphics, MCU definitions, and register access.

```text
                         Application
                         src/main.cpp
                              |
             +----------------+----------------+
             |                |                |
        Sensor drivers    OLED driver      Diagnostics
             |                |                |
        I2C abstraction   Graphics         I2C scanner
             |                |                |
             +----------------+----------------+
                              |
                         STM32 I2C1
                              |
                         MCU registers
                              |
                         STM32F401RE
```

### Sensor dashboard

The dashboard separates measurement acquisition, numeric formatting,
text rendering, and physical display communication.

```text
BMP280::read_measurements()
       |
       +----------------------+
       |                      |
       v                      v
USART2 diagnostics      Sensor Dashboard
                              |
                              v
                        Number formatting
                              |
                              v
                         Text rendering
                              |
                              v
                         Framebuffer
                              |
                              v
                         SH1106 OLED
```

The application functions can share an acquired `Measurements` structure
between USART2 output and the graphical dashboard. These functions are not
called by the current `main()`.

The dashboard does not access I2C directly and does not
perform sensor measurements.

Numeric formatting uses fixed-size character buffers
without dynamic memory allocation.

### Application layer

`src/main.cpp` configures interrupt priority grouping, initializes LD2 and
USART2, creates the heartbeat task, and starts the scheduler. The separate
`sensor_hub` application provides sensor initialization, acquisition, output,
display, diagnostics, and command handling functions; `main()` does not call
them in the current firmware.

The application layer does not contain raw peripheral addresses,
sensor-register addresses, raw measurement decoding, or compensation
formulas.

### Sensor-driver layer

The sensor-driver layer contains device-specific communication and state.

#### BMP280

The BMP280 driver owns:

- I2C device address
- Chip ID register
- Measurement configuration
- Factory-calibration register range
- Raw measurement register range
- Cached calibration state
- High-level measurement acquisition
- Conversion of compensated pressure from Pa to hPa

#### MPU-6050

The MPU-6050 driver owns:

- I2C device address
- Identity register
- Power-management register
- Wake-up procedure
- Measurement register range
- Acceleration conversion
- Temperature conversion
- Angular-velocity conversion
- Gyroscope zero-rate calibration
- Runtime gyroscope bias compensation

Both drivers expose high-level measurement structures containing
physical values.

### Sensor-algorithm layer

`include/sensors/decoding.hpp` provides hardware-independent
`constexpr` helpers for:

- Unsigned 16-bit little-endian decoding
- Signed 16-bit little-endian decoding
- Signed 16-bit big-endian decoding
- Unsigned 20-bit BMP280 measurement decoding

The BMP280 and MPU-6050 drivers share the same decoding implementation.

`src/sensors/bmp280_compensation.cpp` contains the Bosch floating-point
temperature and pressure compensation calculations.

This implementation does not access STM32 registers or I2C peripherals
and can be compiled directly into native unit tests.

### Graphics layer

The graphics system consists of:

```text
Sensor Dashboard
       |
       v
Number formatting
       |
       v
graphics::draw_text()
       |
       v
graphics::draw_char()
       |
       v
graphics::font5x7
       |
       v
drivers::oled::set_pixel()
       |
       v
OLED framebuffer
       |
       v
drivers::oled::flush()
```

The graphics layer converts characters and formatted numeric
values into individual pixels.

The OLED driver owns the framebuffer and the physical transfer
to the display.

The current implementation uses a custom bitmap font rather than
a third-party graphics library.

### Peripheral layer

- `drivers/usart2` — USART2 initialization and text transmission
- `drivers/i2c1` — I2C transactions, probing, diagnostics, and recovery
- `drivers/systick` — standalone millisecond timekeeping and delays; excluded from the FreeRTOS firmware build
- `drivers/oled` — SH1106 initialization, framebuffer, and display transfer

### MCU layer

The MCU layer contains STM32F401 and Cortex-M4 register definitions.

- `mcu/register.hpp` — volatile memory-mapped register access
- `mcu/rcc.hpp` — clocks and peripheral resets
- `mcu/gpio.hpp` — GPIO configuration and state
- `mcu/usart2.hpp` — USART2 register map
- `mcu/i2c1.hpp` — I2C1 register map
- `mcu/systick.hpp` — SysTick register map

## I2C communication

### Register read

A register read sends the internal sensor-register address and changes
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

The API supports one or more consecutive registers.

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

### STM32F4 receive sequences

The STM32F4 I2C peripheral requires different receive sequences depending
on the requested number of bytes.

One byte:

```text
Disable ACK
→ clear ADDR
→ generate STOP
→ wait for RXNE
→ read DR
```

Two bytes:

```text
Set POS
→ disable ACK
→ clear ADDR
→ wait for BTF
→ generate STOP
→ read DR twice
→ restore ACK and POS
```

More than two bytes:

```text
Receive initial bytes with ACK enabled
→ wait for BTF with three bytes remaining
→ disable ACK
→ read byte N-2
→ wait for BTF
→ generate STOP
→ read bytes N-1 and N
→ restore ACK and POS
```

All I2C operations use bounded polling loops to prevent indefinite
waiting when a device does not respond.

### Register write

```text
START
→ device address + write
→ register address
→ value
→ STOP
```

### Generic multi-byte write

The OLED requires a sequence of bytes rather than conventional
sensor-register access.

The I2C1 driver provides:

```cpp
i2c::WriteResult write(
    std::uint8_t address,
    const std::uint8_t *data,
    std::size_t length);
```

A single call transmits the supplied data in one I2C transaction.

```text
START
→ device address + write
→ byte 0
→ byte 1
→ ...
→ byte N
→ STOP
```

The implementation waits for successful transmission before
generating STOP.

Invalid buffers and zero-length transfers are rejected.

### I2C scanner

`diagnostics::i2c_scanner::scan()` probes the usable seven-bit
I2C address range from `0x08` through `0x77`.

The scanner is available through the application command handler, but the
current `main()` does not run that handler.

Verified hardware output:

```text
Scanning I2C bus...
I2C device found at 0x3C
I2C device found at 0x68
I2C device found at 0x76
I2C scan completed
```

### Stuck-bus recovery

Before starting a transaction, the I2C1 driver waits for the STM32
`BUSY` status flag to clear.

If the bus remains busy, the driver attempts physical bus recovery.

```text
Disable I2C1
→ configure PB8/SCL and PB9/SDA as open-drain GPIO
→ release both lines
→ check SDA
→ generate up to 9 SCL pulses
→ generate a manual STOP condition
→ verify SDA and SCL are released high
→ restore I2C1 alternate function
→ reset and reconfigure I2C1
→ check BUSY again
```

The recovery pulses allow a device interrupted during a byte transfer
to advance its internal I2C state machine and release SDA.

The manual STOP condition is generated by releasing SDA from low to
high while SCL is high.

The driver maintains a recovery counter:

```cpp
std::uint32_t recovery_count();
```

The application diagnostics function can report this counter through USART2;
it is not called by the current `main()`.

The recovery mechanism was tested by temporarily forcing SDA low.
After releasing SDA, BMP280 and MPU-6050 communication resumed without
resetting the MCU.

This test verifies recovery from a persistent physical bus fault.
It does not emulate every possible internal device state.

## SH1106 OLED display

### Hardware

| Property | Value |
|----------|-------|
| Display type | Monochrome OLED |
| Size | 1.30 inch |
| Resolution | 128×64 |
| Controller | SH1106 |
| Interface | I2C |
| Address | `0x3C` |
| Supply | 3.3 V |

The display shares I2C1 with BMP280 and MPU-6050.

The firmware controls the display directly without HAL, Arduino,
or a third-party graphics library.

### Command transmission

SH1106 distinguishes commands from graphical data using an
I2C control byte.

Command transmission:

```text
START
→ device address + write
→ 0x00
→ command
→ STOP
```

Example:

```cpp
const std::uint8_t command[] = {
    0x00U,
    0xAFU
};

drivers::i2c1::write(
    0x3CU,
    command,
    sizeof(command));
```

`0x00` identifies the following byte as a command.

`0xAF` enables the display.

### Data transmission

Graphical data use control byte `0x40`.

```text
START
→ device address + write
→ 0x40
→ display data
→ STOP
```

The current implementation transfers display data in chunks of
16 framebuffer bytes, preceded by the control byte.

### Initialization

The OLED driver configures:

- Display ON/OFF
- Display clock
- Multiplex ratio
- Display offset
- Start line
- Segment remapping
- COM scan direction
- Contrast
- Normal display mode
- Charge pump

Display-control API:

```cpp
bool initialize();

bool display_all_on();

bool display_normal();
```

`display_all_on()` uses SH1106 command `0xA5` to illuminate
the entire display independently of framebuffer contents.

This command was used during the initial physical OLED bring-up.

`display_normal()` restores normal framebuffer-based rendering.

### Framebuffer

The OLED uses a monochrome 128×64 framebuffer.

Each pixel occupies one bit.

```text
128 × 64 / 8 = 1024 bytes
```

The framebuffer is stored in STM32 SRAM.

```cpp
std::uint8_t framebuffer[1024]{};
```

The display memory is organized into eight pages.

```text
Page 0: rows  0–7
Page 1: rows  8–15
Page 2: rows 16–23
Page 3: rows 24–31
Page 4: rows 32–39
Page 5: rows 40–47
Page 6: rows 48–55
Page 7: rows 56–63
```

Each page contains 128 visible columns.

The framebuffer index is calculated as:

```cpp
index = x + (y / 8U) * 128U;
```

The pixel bit is:

```cpp
bit = 1U << (y % 8U);
```

Setting a pixel modifies the corresponding framebuffer byte:

```cpp
framebuffer[index] |= bit;
```

The SH1106 contains 132 columns of internal display RAM.
The current module uses a two-column offset to align the
128 visible columns correctly.

### Framebuffer API

```cpp
void clear();

void set_pixel(
    std::uint8_t x,
    std::uint8_t y);

bool flush();
```

`clear()` resets the framebuffer in SRAM.

`set_pixel()` updates an individual pixel and ignores coordinates
outside the display boundaries.

`flush()` transfers the framebuffer to SH1106.

The transfer uses page addressing and 16-byte data chunks.

```text
Framebuffer
    |
    +-- Page 0
    |      +-- 8 data chunks
    |
    +-- Page 1
    |      +-- 8 data chunks
    |
    ...
    |
    +-- Page 7
           +-- 8 data chunks
```

A complete framebuffer update transmits 1024 graphical data bytes,
plus I2C control bytes and addressing commands.

The driver stops the transfer if an I2C operation fails.

## Graphics and text rendering

### Font

`include/graphics/font5x7.hpp` defines the custom bitmap font.

Each glyph contains five vertical columns.

Each column is represented by one byte, with seven bits used
for visible pixels.

```cpp
struct Glyph {
  char character;
  std::uint8_t columns[width];
};
```

Font dimensions:

```cpp
constexpr std::uint8_t width = 5U;
constexpr std::uint8_t height = 7U;
constexpr std::uint8_t spacing = 1U;
```

The current font contains:

- Space
- Digits `0–9`
- Uppercase Latin alphabet `A–Z`
- Lowercase Latin alphabet `a–z`

Total: 63 glyphs.

### Text rendering

The graphics API provides:

```cpp
void draw_char(
    std::uint8_t x,
    std::uint8_t y,
    char character);

void draw_text(
    std::uint8_t x,
    std::uint8_t y,
    const char *text);
```

`draw_char()` locates the requested glyph and translates its
bitmap columns into framebuffer pixels.

`draw_text()` renders consecutive characters using the font
width and spacing.

Horizontal coordinates use a wider integer type internally
to avoid eight-bit coordinate wraparound.

Rendering stops when the next glyph would exceed the display width.

Unsupported characters are currently skipped.

### Verified graphical output

The OLED was first tested using the SH1106 entire-display-on command.

The next hardware test verified framebuffer addressing by
rendering graphical primitives.

The text-rendering milestone was verified using:

```cpp
drivers::oled::clear();

graphics::draw_text(20U, 20U, "VIKUSIA");
graphics::draw_text(20U, 32U, "SENSOR HUB");

drivers::oled::flush();
```

The text was displayed successfully on physical hardware.

The sensor hub initialization function renders the startup message when
called. It is not called by the current `main()`.

## Number formatting

`graphics::format_fixed_2()` converts floating-point measurements
into decimal text with two fractional digits.

The implementation does not use `sprintf`, dynamic allocation,
or the standard I/O formatting library.

```cpp
bool format_fixed_2(
    float value,
    char *buffer,
    std::size_t size);
```

Example:

```cpp
char temperature[16]{};

if (graphics::format_fixed_2(
        measurements.temperature_c,
        temperature,
        sizeof(temperature))) {

  graphics::draw_text(42U, 20U, temperature);
}
```

For a measurement of `25.89`, the resulting string is:

```text
25.89
```

The formatter:

- Supports positive and negative values
- Produces two fractional digits
- Rounds to two decimal places
- Checks output buffer capacity
- Rejects null and empty buffers
- Rejects NaN and infinity
- Rejects values outside the supported range

The same production implementation is compiled into the
STM32 firmware and native host-side tests.

## Sensor dashboard implementation

The dashboard is implemented in:

```text
include/graphics/sensor_dashboard.hpp
src/graphics/sensor_dashboard.cpp
```

Its public API is:

```cpp
namespace graphics::sensor_dashboard {

bool render(
    const drivers::bmp280::Measurements &bmp280_measurements,
    bool bmp280_valid,
    const drivers::mpu6050::Measurements &mpu6050_measurements,
    bool mpu6050_valid);

}
```

The dashboard receives already acquired measurements.

It does not initialize the BMP280 or perform I2C transactions.

The rendering sequence is:

```text
BMP280 Measurements
        |
        v
Format temperature
        |
        v
Format pressure
        |
        v
Clear framebuffer
        |
        v
Draw header and values
        |
        v
Return rendering status
```

The `application::sensor_hub::display_sample()` function renders a sample
and then flushes the framebuffer. `acquire_sample()` reads each ready sensor,
and `report_sample()` reports valid measurements or an unavailable message.
The dashboard receives validity flags for both sensors. Rendering and flush
failures are reported through USART2. These application functions are not
scheduled in the current firmware.

## BMP280

### Identification and configuration

| Property | Value |
|----------|-------|
| I2C address | `0x76` |
| Chip ID register | `0xD0` |
| Expected chip ID | `0x58` |
| `CTRL_MEAS` register | `0xF4` |
| `CTRL_MEAS` value | `0x27` |
| Calibration range | `0x88–0x9F` |
| Calibration size | 24 bytes |
| Measurement range | `0xF7–0xFC` |
| Measurement size | 6 bytes |

`CTRL_MEAS = 0x27` configures:

| Field | Value | Configuration |
|-------|-------|---------------|
| `osrs_t` | `001` | Temperature oversampling ×1 |
| `osrs_p` | `001` | Pressure oversampling ×1 |
| `mode` | `11` | Normal mode |

The driver reads `CTRL_MEAS` back after writing it and fails
initialization if the value does not match.

### Factory calibration

The BMP280 contains 12 factory-programmed coefficients:

```text
dig_T1, dig_T2, dig_T3
dig_P1, dig_P2, dig_P3, dig_P4, dig_P5
dig_P6, dig_P7, dig_P8, dig_P9
```

They occupy 24 consecutive bytes from `0x88` through `0x9F`.

Calibration words use little-endian byte order.

`dig_T1` and `dig_P1` are unsigned 16-bit values.

The remaining coefficients are signed 16-bit two's-complement values.

Calibration is loaded once during initialization and cached in RAM.

### Raw measurements

Pressure and temperature are read in one six-byte I2C transaction.

| Register range | Measurement |
|----------------|-------------|
| `0xF7–0xF9` | Pressure |
| `0xFA–0xFC` | Temperature |

Each raw measurement occupies 20 bits.

```text
MSB[7:0] + LSB[7:0] + XLSB[7:4]
```

The value is assembled as:

```cpp
(msb << 12U) | (lsb << 4U) | (xlsb >> 4U)
```

### Compensation

The firmware applies the Bosch floating-point compensation formulas.

```text
Raw temperature
+ dig_T1–dig_T3
→ t_fine
→ temperature [°C]
```

```text
Raw pressure
+ t_fine
+ dig_P1–dig_P9
→ pressure [Pa]
→ pressure [hPa]
```

High-level API:

```cpp
struct Measurements {
  float temperature_c;
  float pressure_hpa;
};

bool read_measurements(Measurements &measurements);
```

BMP280 pressure is the absolute pressure at the sensor location.

Weather services often report pressure corrected to sea level,
so the values may differ.

## MPU-6050

### Identification and initialization

| Property | Value |
|----------|-------|
| I2C address | `0x68` |
| `WHO_AM_I` register | `0x75` |
| Expected identity | `0x68` |
| `PWR_MGMT_1` register | `0x6B` |
| Measurement start register | `0x3B` |
| Measurement frame size | 14 bytes |
| Sleep bit | Bit 6 |

The driver writes `0x00` to `PWR_MGMT_1` and reads it back
to verify that the sensor is awake.

After wake-up verification, the firmware calibrates the
gyroscope zero-rate offset.

### Measurement frame

A complete frame is read through one 14-byte I2C transaction.

| Register range | Bytes | Measurement |
|----------------|-------|-------------|
| `0x3B–0x40` | 6 | Accelerometer X, Y, Z |
| `0x41–0x42` | 2 | Temperature |
| `0x43–0x48` | 6 | Gyroscope X, Y, Z |

Each measurement is a signed 16-bit two's-complement value
with the high byte transmitted first.

### Conversion formulas

The current configuration uses the default full-scale ranges:

- Accelerometer: ±2 g
- Gyroscope: ±250 degrees per second

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

The temperature represents the internal sensor temperature
rather than a precise ambient-air measurement.

### Gyroscope calibration

During initialization, the firmware collects 100 uncorrected
gyroscope samples while the board is stationary.

For each axis:

```text
bias = sum of uncorrected samples / sample count
```

Normal measurement reads subtract the corresponding bias:

```text
corrected angular velocity = measured angular velocity - bias
```

Calibration uses a separate uncorrected measurement path.

The board must remain stationary during startup calibration.

After calibration, a stationary sensor reports angular velocity
close to zero while preserving its response to actual rotation.

## FreeRTOS execution

The firmware uses FreeRTOS Kernel V11.1.0 with the `GCC_ARM_CM4F` port on
the STM32F401RE Cortex-M4F. The build uses the single-precision FPU and hard
float ABI. `Reset_Handler` enables the FPU, initializes `.data` and `.bss`,
runs C++ global constructors, and calls `main()`.

### Startup and tasks

`main()` assigns all implemented interrupt priority bits to preemption,
configures PA5/LD2 as an output initially off, initializes USART2, and sends
`Stm32SensorHub FreeRTOS starting`. It creates the heartbeat task with
`xTaskCreateStatic()` and calls `vTaskStartScheduler()`. FreeRTOS also creates
its idle task. Initialization failure, task creation failure, or a scheduler
return halts the firmware with interrupts disabled and a diagnostic reason.

The heartbeat task has priority 1 and toggles LD2 immediately on its first run,
then every 1,000 ms. It records its minimum remaining stack space in words in
`heartbeat_stack_free_words`. It uses `vTaskDelayUntil()` with a previous wake
time, so releases follow an absolute tick schedule rather than accumulating
execution time as a relative delay would. Actual execution can be delayed by
interrupts or higher-priority work. The idle task runs when no other task is
ready. Preemption is enabled, and four task priorities are configured.

### Tick and exceptions

FreeRTOS owns SysTick in the firmware build. The configuration specifies a
16 MHz CPU clock and a 1,000 Hz tick (1 ms; SysTick reload 15,999), with a
32-bit tick count. The port supplies `SysTick_Handler`, `PendSV_Handler`, and
`SVC_Handler` through the names in `FreeRTOSConfig.h` and the startup vector
table. The standalone `drivers/systick.cpp` is excluded from the firmware
build; its handler and time counter are not used alongside FreeRTOS.

### Interrupt priorities

The configuration uses four implemented priority bits. SysTick and PendSV use
library priority 15, the lowest priority. `configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY`
is 5: interrupts at numerical priorities 5–15 may use permitted FreeRTOS
`FromISR` APIs, while more urgent priorities 0–4 must not call them. USART2
and DMA1 Stream6 are configured at priority 6; their current handlers do not
call FreeRTOS APIs. Lower numerical values represent higher urgency.

### Allocation and failure handling

Dynamic FreeRTOS allocation is disabled. The heartbeat task has a static
control block and a 256-word stack; the idle task's static control block and
256-word stack are supplied by `vApplicationGetIdleTaskMemory()`. Software
timers, the idle hook, and the tick hook are disabled. Stack overflow checking
is enabled; its hook records the task and name before halting. Assertions
record the source file and line before halting. A malloc failure hook is also
provided, although dynamic FreeRTOS allocation is disabled.

### Current runtime

After startup, the heartbeat is the only application task. USART2 and its DMA
and receive interrupt infrastructure are initialized for the startup message,
but console commands are not processed. I2C initialization, sensor sampling,
serial measurement reports, diagnostics, and OLED initialization and updates
are not active. The corresponding drivers and application functions remain in
the source tree.

## Floating-point support

The firmware uses the STM32F401 single-precision
hardware floating-point unit.

Before entering `main()`, `Reset_Handler` enables access
to Cortex-M4 coprocessors CP10 and CP11 through `SCB_CPACR`.

The initialization is followed by:

```asm
dsb
isb
```

Without FPU initialization, executing a floating-point
instruction causes a `NOCP` UsageFault.

## Host-side unit tests

Hardware-independent sensor logic is tested natively
on Fedora using GoogleTest and CTest.

```text
STM32 firmware
    arm-none-eabi-g++
    |
    build/

Host-side tests
    native g++
    + GoogleTest
    |
    build-tests/
```

### Sensor decoding tests

The tests verify:

- Unsigned 16-bit little-endian decoding
- Signed positive 16-bit little-endian decoding
- Signed negative 16-bit little-endian decoding
- Signed 16-bit big-endian decoding
- Unsigned 20-bit BMP280 decoding

### BMP280 compensation tests

Reference values:

```text
Raw temperature = 519888
Raw pressure    = 415148
```

Expected compensated results:

```text
Temperature ≈ 25.08 °C
Pressure    ≈ 100653 Pa
```

The production compensation implementation is compiled directly
into the native test executable.

### I2C dependency injection

Sensor drivers depend on the hardware-independent
`I2cBus` abstraction.

```cpp
struct I2cBus {
  ReadResult (*read_register)(
      std::uint8_t,
      std::uint8_t,
      std::uint8_t &);

  ReadResult (*read_registers)(
      std::uint8_t,
      std::uint8_t,
      std::uint8_t *,
      std::size_t);

  WriteResult (*write_register)(
      std::uint8_t,
      std::uint8_t,
      std::uint8_t);
};
```

The application injects the production STM32 implementation:

```cpp
constexpr drivers::i2c::I2cBus i2c_bus{
    .read_register = drivers::i2c1::read_register,
    .read_registers = drivers::i2c1::read_registers,
    .write_register = drivers::i2c1::write_register,
};

drivers::bmp280::set_i2c_bus(i2c_bus);
drivers::mpu6050::set_i2c_bus(i2c_bus);
```

Host-side tests inject `MockI2c`.

```text
                +-- STM32 I2C1 -- MCU registers
Sensor driver -- I2cBus
                +-- MockI2c ---- Host tests
```

This allows production sensor-driver code to be tested
without physical STM32 hardware.

The design avoids runtime virtual polymorphism.

### Sensor-driver tests

BMP280 tests verify:

- Successful chip-ID reading
- I2C read-failure propagation

MPU-6050 tests verify:

- Successful `WHO_AM_I` reading
- I2C read-failure propagation
- Wake-up register writes

The wake-up test verifies the transition:

```text
PWR_MGMT_1 = 0x40
       ↓
PWR_MGMT_1 = 0x00
```

### Number formatting tests

`tests/number_format_test.cpp` contains 13 GoogleTest cases.

The tests verify:

- Positive temperature formatting
- Pressure formatting
- Negative values
- Zero
- Rounding to two decimal places
- Small values
- Null buffer rejection
- Empty buffer rejection
- Insufficient buffer rejection
- Exact buffer-size handling
- NaN rejection
- Infinity rejection
- Out-of-range value rejection

The production `number_format.cpp` implementation is compiled
directly into the native test executable.

### Configure and run tests

```bash
cmake -S tests -B build-tests -G Ninja
```

```bash
cmake --build build-tests
```

```bash
ctest --test-dir build-tests --output-on-failure
```

The native test build is separate from the ARM firmware build.

The current test suite contains 25 tests.

## Serial startup output

The current firmware sends the following message at startup:

```text
Stm32SensorHub FreeRTOS starting
```

## Requirements

### Firmware

- Fedora Linux
- CMake 3.28 or newer
- Ninja
- `arm-none-eabi-gcc`
- `arm-none-eabi-g++`
- OpenOCD
- picocom

### Host-side tests

- Native C++20 compiler
- GoogleTest
- CTest

## Configure firmware

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

Exit picocom:

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
