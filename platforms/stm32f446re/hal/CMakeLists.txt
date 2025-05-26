add_library(
    stm32f4_hal
    stm32f4_gpio.c
    stm32f4_systick.c
    stm32f4_uart.c
)
target_link_libraries(stm32f4_hal PUBLIC hal_interface PRIVATE stm32f446re_sdk)
