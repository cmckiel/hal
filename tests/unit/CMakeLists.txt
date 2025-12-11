add_executable(
    desktop_unit_tests
    gpio_driver_test.cpp
    i2c_driver_test.cpp
    i2c_transaction_queue_test.cpp
    pwm_driver_test.cpp
    uart_driver_test.cpp
    uart1_driver_test.cpp
    uart2_driver_test.cpp
    main.cpp
)

target_include_directories(
    desktop_unit_tests
    PRIVATE
    ${CMAKE_SOURCE_DIR}/src/include
    ${CMAKE_SOURCE_DIR}/src/i2c/include
    ${CMAKE_SOURCE_DIR}/src/uart/include
)

target_link_libraries(
    desktop_unit_tests
    hal_interface
    stm32f4_hal
    stm32f4_mock
    circular_buffer
    GTest::gtest
    GTest::gtest_main
)

enable_testing()

include(GoogleTest)
gtest_discover_tests(desktop_unit_tests)
