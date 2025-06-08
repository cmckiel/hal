enable_testing()

# Mock stm32f4 hardware.
add_library(stm32f4_mock
    stm32f4_mocks/src/registers.c
)

target_include_directories(stm32f4_mock PUBLIC stm32f4_mocks/include)

# The real hardware driver compiled for desktop.
add_library(uart2_driver
    ../../platforms/stm32f446re/hal/uart/src/stm32f4_uart2.c
)

target_compile_definitions(uart2_driver PRIVATE SIMULATION_BUILD)

# GTest
find_package(GTest REQUIRED)

add_executable(uart2_driver_test
    test/uart2_driver_test.cpp
    test/main.cpp
)

target_link_libraries(uart2_driver_test
    uart2_driver
    stm32f4_mock
    GTest::GTest
    GTest::Main
)

add_test(NAME uart2_driver_test COMMAND uart2_driver_test)
