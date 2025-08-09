add_subdirectory(stm32f4_mocks)

# The real hardware driver compiled for desktop.
add_library(
    uart_driver
    ../../platforms/stm32f446re/hal/uart/src/stm32f4_uart_util.c
    ../../platforms/stm32f446re/hal/uart/src/stm32f4_uart.c
    ../../platforms/stm32f446re/hal/uart/src/stm32f4_uart1.c
    ../../platforms/stm32f446re/hal/uart/src/stm32f4_uart2.c
)

add_library(
    i2c_driver
    ../../platforms/stm32f446re/hal/i2c/src/stm32f4_i2c.c
    ../../platforms/stm32f446re/hal/i2c/src/i2c_message_queue.c
    ../../platforms/stm32f446re/hal/i2c/src/i2c_state_machine.c
)

target_compile_definitions(
    uart_driver
    PRIVATE
    SIMULATION_BUILD
)

target_compile_definitions(
    i2c_driver
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

target_include_directories(
    i2c_driver
    PUBLIC
    ../../hal_interface/include
    ../../platforms/stm32f446re/hal/include
    ../../platforms/stm32f446re/hal/i2c/include
)

target_link_libraries(
    uart_driver
    PRIVATE
    stm32f4_mock
    circular_buffer
)

target_link_libraries(
    i2c_driver
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

add_executable(
    i2c_driver_test
    test/i2c_driver_test.cpp
    test/i2c_state_machine_test.cpp
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

target_link_libraries(
    i2c_driver_test
    i2c_driver
    stm32f4_mock
    circular_buffer
    GTest::gtest
    GTest::gtest_main
)

enable_testing()

include(GoogleTest)
gtest_discover_tests(uart_driver_test)
gtest_discover_tests(i2c_driver_test)
