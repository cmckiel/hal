# Mock stm32f4 hardware.
add_library(
    stm32f4_mock
    src/registers.c
)

target_include_directories(
    stm32f4_mock
    PUBLIC
    include
)
