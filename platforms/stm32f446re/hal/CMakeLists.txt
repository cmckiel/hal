add_library(
    stm32f4_hal
    stm32f4_gpio.c
    stm32f4_systick.c
    uart/src/stm32f4_uart.c
    uart/src/stm32f4_uart1.c
    uart/src/stm32f4_uart2.c
)
target_link_libraries(stm32f4_hal PUBLIC hal_interface PRIVATE stm32f446re_sdk)
target_include_directories(stm32f4_hal PRIVATE ./uart/include)
