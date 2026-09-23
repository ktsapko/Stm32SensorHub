cmake_minimum_required(VERSION 3.28)

project(Stm32SensorHubTests LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_testing()

find_package(GTest REQUIRED)

add_executable(decoding_tests
    decoding_test.cpp
)

target_include_directories(decoding_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(decoding_tests PRIVATE
    GTest::gtest_main
)

include(GoogleTest)
gtest_discover_tests(decoding_tests)

add_executable(bmp280_compensation_tests
    bmp280_compensation_test.cpp
    ../src/sensors/bmp280_compensation.cpp
)

target_include_directories(bmp280_compensation_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/../include
)

target_link_libraries(bmp280_compensation_tests PRIVATE
    GTest::gtest_main
)

gtest_discover_tests(bmp280_compensation_tests)

add_executable(
    bmp280_driver_tests
    bmp280_driver_test.cpp
    ../src/drivers/bmp280.cpp
    ../src/sensors/bmp280_compensation.cpp
)

target_include_directories(
    bmp280_driver_tests
    PRIVATE
    ../include
    .
)

target_link_libraries(
    bmp280_driver_tests
    PRIVATE
    GTest::gtest_main
)

gtest_discover_tests(bmp280_driver_tests)

add_executable(
    mpu6050_driver_tests
    mpu6050_driver_test.cpp
    ../src/drivers/mpu6050.cpp
)

target_include_directories(
    mpu6050_driver_tests
    PRIVATE
    ../include
    .
)

target_link_libraries(
    mpu6050_driver_tests
    PRIVATE
    GTest::gtest_main
)

gtest_discover_tests(mpu6050_driver_tests)

add_executable(
    number_format_tests
    number_format_test.cpp
    ../src/graphics/number_format.cpp
)

target_include_directories(
    number_format_tests
    PRIVATE
    ../include
)

target_link_libraries(
    number_format_tests
    PRIVATE
    GTest::gtest_main
)

gtest_discover_tests(number_format_tests)


add_executable(
    ring_buffer_tests
    ring_buffer_test.cpp
)

target_include_directories(
    ring_buffer_tests
    PRIVATE
    ../include
)

target_link_libraries(
    ring_buffer_tests
    PRIVATE
    GTest::gtest_main
)

gtest_discover_tests(ring_buffer_tests)
