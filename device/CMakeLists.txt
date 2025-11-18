# Add Device Support Package
add_library(
    stm32f4_device_support_package
    src/syscalls.c
    src/sysmem.c
    startup/startup_stm32f446retx.s
)

target_include_directories(
    stm32f4_device_support_package
    PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include/cmsis
    ${CMAKE_CURRENT_LIST_DIR}/include/stm32f4
)
