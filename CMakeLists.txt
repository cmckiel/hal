cmake_minimum_required(VERSION 3.20)

set(CMAKE_TOOLCHAIN_FILE "./cmake/toolchain/CMakeToolChain.stm32f446.cmake")

set(PROJECT_NAME "hal")

project(${PROJECT_NAME} LANGUAGES C ASM CXX)

add_subdirectory(hal_interface)
add_subdirectory(platforms)

add_executable(
    ${PROJECT_NAME}.elf
    test/main.c
)

target_link_libraries(${PROJECT_NAME}.elf stm32f4_hal)

add_custom_target(${PROJECT_NAME}.bin ALL
    arm-none-eabi-objcopy -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
    DEPENDS ${PROJECT_NAME}.elf
)
