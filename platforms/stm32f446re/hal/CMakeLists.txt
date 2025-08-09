add_library(
    stm32f4_hal
    stm32f4_gpio.c
    stm32f4_systick.c
    i2c/src/i2c_message_queue.c
    i2c/src/i2c_state_machine.c
    i2c/src/stm32f4_i2c.c
    uart/src/stm32f4_uart.c
    uart/src/stm32f4_uart1.c
    uart/src/stm32f4_uart2.c
    uart/src/stm32f4_uart_util.c
)

# Link against the mocks for static analysis to avoid analyzing vendor sdk.
if (STATIC_ANALYSIS_BUILD)
    target_link_libraries(stm32f4_hal PUBLIC hal_interface PRIVATE stm32f4_mock circular_buffer)
    target_compile_definitions(stm32f4_hal PRIVATE SIMULATION_BUILD)
else()
    target_link_libraries(stm32f4_hal PUBLIC hal_interface PRIVATE stm32f446re_sdk circular_buffer)
endif()

target_include_directories(stm32f4_hal PRIVATE ./uart/include ./include ./i2c/include)
