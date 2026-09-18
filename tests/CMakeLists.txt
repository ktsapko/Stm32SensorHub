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