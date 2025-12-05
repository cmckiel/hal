# Add all the source files of our HAL to the stm32f4_hal library.
add_library(
    stm32f4_hal
    gpio/stm32f4_gpio.c
    i2c/src/i2c_transaction_queue.c
    i2c/src/stm32f4_i2c.c
    metadata/src/hal_metadata.c
    pwm/src/stm32f4_pwm.c
    uart/src/stm32f4_uart.c
    uart/src/stm32f4_uart1.c
    uart/src/stm32f4_uart2.c
    uart/src/stm32f4_uart_util.c
    system/stm32f4_hal_system.c
    systick/stm32f4_systick.c
)

# Include all the directories containing headers.
target_include_directories(
    stm32f4_hal
    PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${CMAKE_CURRENT_LIST_DIR}/i2c/include
    ${CMAKE_CURRENT_LIST_DIR}/uart/include
    ${CMAKE_BINARY_DIR}/generated
)

# Link to the common libraries.
target_link_libraries(
    stm32f4_hal
    PUBLIC hal_interface
    PRIVATE circular_buffer
)

# Either use the real device support package for embedded builds or swap
# in the mocks for desktop builds.
if (DESKTOP_BUILD)
    target_link_libraries(
        stm32f4_hal
        PRIVATE stm32f4_mock
    )
    # Inject DESKTOP_BUILD flag into build so files can
    # choose appropriate mock headers.
    target_compile_definitions(
        stm32f4_hal
        PRIVATE DESKTOP_BUILD
    )
else()
    target_link_libraries(
        stm32f4_hal
        PRIVATE stm32f4_device_support_package
    )
endif()
