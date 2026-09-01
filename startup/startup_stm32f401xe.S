.syntax unified
.cpu cortex-m4
.fpu fpv4-sp-d16
.thumb

.global g_vector_table
.global Reset_Handler
.global Default_Handler

.extern main
.extern __libc_init_array

.section .isr_vector, "a", %progbits
.type g_vector_table, %object

g_vector_table:
    .word _estack
    .word Reset_Handler

    /* 14 системних обробників Cortex-M4 */
    .rept 14
        .word Default_Handler
    .endr

    /* 85 зовнішніх переривань STM32F401 */
    .rept 85
        .word Default_Handler
    .endr

.size g_vector_table, . - g_vector_table

.section .text.Reset_Handler, "ax", %progbits
.type Reset_Handler, %function

Reset_Handler:
    /* Копіювання секції .data із Flash у SRAM */
    ldr r0, =_sidata
    ldr r1, =_sdata
    ldr r2, =_edata

copy_data:
    cmp r1, r2
    bcs zero_bss

    ldr r3, [r0], #4
    str r3, [r1], #4
    b copy_data

    /* Обнулення секції .bss */
zero_bss:
    ldr r1, =_sbss
    ldr r2, =_ebss
    movs r3, #0

clear_bss:
    cmp r1, r2
    bcs initialize_cpp

    str r3, [r1], #4
    b clear_bss

    /* Виклик конструкторів глобальних C++-об’єктів */
initialize_cpp:
    bl __libc_init_array

    /* Перехід у програму */
    bl main

hang:
    b hang

.size Reset_Handler, . - Reset_Handler

.section .text.Default_Handler, "ax", %progbits
.type Default_Handler, %function

Default_Handler:
    b Default_Handler

.size Default_Handler, . - Default_Handler