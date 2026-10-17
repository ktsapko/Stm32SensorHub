#pragma once

#define configCPU_CLOCK_HZ 16000000UL
#define configTICK_RATE_HZ 1000U
#define configUSE_PREEMPTION 1

#define configSUPPORT_STATIC_ALLOCATION  1
#define configSUPPORT_DYNAMIC_ALLOCATION 0

#define configMINIMAL_STACK_SIZE        256U
#define configMAX_PRIORITIES            4U
#define configMAX_TASK_NAME_LEN          16U

#define vPortSVCHandler     SVC_Handler
#define xPortPendSVHandler  PendSV_Handler
#define xPortSysTickHandler SysTick_Handler

#define configPRIO_BITS 4U

#define configLIBRARY_LOWEST_INTERRUPT_PRIORITY      15U
#define configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY 5U

#define configKERNEL_INTERRUPT_PRIORITY \
    (configLIBRARY_LOWEST_INTERRUPT_PRIORITY << (8U - configPRIO_BITS))

#define configMAX_SYSCALL_INTERRUPT_PRIORITY \
    (configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY << (8U - configPRIO_BITS))

#define configTICK_TYPE_WIDTH_IN_BITS TICK_TYPE_WIDTH_32_BITS
#define configUSE_TIMERS              0
#define configUSE_IDLE_HOOK           0
#define configUSE_TICK_HOOK           0

#define INCLUDE_vTaskDelayUntil       1
#define INCLUDE_uxTaskGetStackHighWaterMark 1

#define configCHECK_FOR_STACK_OVERFLOW 2
#define configUSE_MALLOC_FAILED_HOOK   1

#ifdef __cplusplus
extern "C" {
#endif

void freertos_assert_failed(const char *file, int line);

#ifdef __cplusplus
}
#endif

#define configASSERT(condition)                       \
    do {                                              \
        if (!(condition)) {                           \
            freertos_assert_failed(__FILE__, __LINE__); \
        }                                             \
    } while (0)
