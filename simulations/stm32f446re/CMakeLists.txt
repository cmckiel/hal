add_subdirectory(stm32f4_mocks)

# The real hardware driver compiled for desktop.
add_library(
    uart2_driver
    ../../platforms/stm32f446re/hal/uart/src/stm32f4_uart.c
    ../../platforms/stm32f446re/hal/uart/src/stm32f4_uart1.c
    ../../platforms/stm32f446re/hal/uart/src/stm32f4_uart2.c
)

target_compile_definitions(
    uart2_driver
    PRIVATE
    SIMULATION_BUILD
)

target_include_directories(
    uart2_driver
    PUBLIC
    ../../platforms/stm32f446re/hal/uart/include
    ../../hal_interface/include
    ../../platforms/stm32f446re/hal/include
)

target_link_libraries(
    uart2_driver
    PRIVATE
    stm32f4_mock
    circular_buffer
)

add_executable(
    uart2_driver_test
    test/uart2_driver_test.cpp
    test/main.cpp
)

target_link_libraries(
    uart2_driver_test
    uart2_driver
    stm32f4_mock
    GTest::gtest
    GTest::gtest_main
)

enable_testing()

include(GoogleTest)
gtest_discover_tests(uart2_driver_test)
