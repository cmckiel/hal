add_subdirectory(stm32f4_mocks)

# The real hardware driver compiled for desktop.
add_library(
    uart_driver
    ../../platforms/stm32f446re/hal/uart/src/stm32f4_uart_util.c
    ../../platforms/stm32f446re/hal/uart/src/stm32f4_uart.c
    ../../platforms/stm32f446re/hal/uart/src/stm32f4_uart1.c
    ../../platforms/stm32f446re/hal/uart/src/stm32f4_uart2.c
)

target_compile_definitions(
    uart_driver
    PRIVATE
    SIMULATION_BUILD
)

target_include_directories(
    uart_driver
    PUBLIC
    ../../platforms/stm32f446re/hal/uart/include
    ../../hal_interface/include
    ../../platforms/stm32f446re/hal/include
)

target_link_libraries(
    uart_driver
    PRIVATE
    stm32f4_mock
    circular_buffer
)

add_executable(
    uart_driver_test
    test/uart_driver_test.cpp
    test/uart1_driver_test.cpp
    test/uart2_driver_test.cpp
    test/main.cpp
)

target_link_libraries(
    uart_driver_test
    uart_driver
    stm32f4_mock
    circular_buffer
    GTest::gtest
    GTest::gtest_main
)

enable_testing()

include(GoogleTest)
gtest_discover_tests(uart_driver_test)
