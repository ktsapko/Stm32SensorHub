include(FetchContent)

add_library(freertos_config INTERFACE)
target_include_directories(freertos_config INTERFACE
    ${PROJECT_SOURCE_DIR}/config
)
target_link_libraries(freertos_config INTERFACE stm32_cpu)

set(FREERTOS_PORT GCC_ARM_CM4F CACHE STRING "FreeRTOS port")

FetchContent_Declare(freertos_kernel
    GIT_REPOSITORY https://github.com/FreeRTOS/FreeRTOS-Kernel.git
    GIT_TAG dbf70559b27d39c1fdb68dfb9a32140b6a6777a0 # V11.1.0
    GIT_SUBMODULES ""
)
FetchContent_MakeAvailable(freertos_kernel)
