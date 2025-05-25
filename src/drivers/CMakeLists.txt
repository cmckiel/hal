add_library(
    drivers
    src/uart.c
    src/gpio.c
    src/adc.c
    src/systick.c
)

target_include_directories(drivers PUBLIC ./include)
target_link_libraries(drivers stm32f446re_dev_kit)