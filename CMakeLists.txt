cmake_minimum_required(VERSION 3.28)

project(Stm32SensorHub LANGUAGES C CXX ASM)

set(CMAKE_C_STANDARD 17)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(TARGET_NAME stm32_sensor_hub)
set(LINKER_SCRIPT
    ${CMAKE_SOURCE_DIR}/linker/STM32F401RETx_FLASH.ld
)

add_executable(${TARGET_NAME}
    startup/startup_stm32f401xe.S
    src/main.cpp
    src/drivers/usart2.cpp
)

target_include_directories(${TARGET_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/include
)

target_compile_options(${TARGET_NAME} PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard

    -Og
    -g3

    -ffunction-sections
    -fdata-sections
    -fno-common

    -Wall
    -Wextra
    -Wpedantic
    -Wconversion
    -Wshadow

    $<$<COMPILE_LANGUAGE:CXX>:
        -fno-exceptions
        -fno-rtti
        -fno-threadsafe-statics
    >
)

target_link_options(${TARGET_NAME} PRIVATE
    -mcpu=cortex-m4
    -mthumb
    -mfpu=fpv4-sp-d16
    -mfloat-abi=hard

    -T${LINKER_SCRIPT}

    --specs=nano.specs
    --specs=nosys.specs

    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -Wl,-Map=${CMAKE_BINARY_DIR}/${TARGET_NAME}.map
)

set_target_properties(${TARGET_NAME} PROPERTIES
    SUFFIX ".elf"
)

add_custom_command(
    TARGET ${TARGET_NAME}
    POST_BUILD

    COMMAND ${CMAKE_OBJCOPY}
        -O binary
        $<TARGET_FILE:${TARGET_NAME}>
        ${CMAKE_BINARY_DIR}/${TARGET_NAME}.bin

    COMMAND ${CMAKE_OBJCOPY}
        -O ihex
        $<TARGET_FILE:${TARGET_NAME}>
        ${CMAKE_BINARY_DIR}/${TARGET_NAME}.hex

    COMMAND ${CMAKE_SIZE}
        $<TARGET_FILE:${TARGET_NAME}>

    COMMENT "Generating BIN and HEX firmware images"
)