# Add Device Support Package
add_library(
    stm32f4_device_support_package STATIC
    src/syscalls.c
    src/sysmem.c
    startup/startup_stm32f446retx.s
)

target_include_directories(
    stm32f4_device_support_package
    PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/cmsis>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include/stm32f4>
    $<INSTALL_INTERFACE:include/stm32f4_device_support_package/cmsis>
    $<INSTALL_INTERFACE:include/stm32f4_device_support_package/stm32f4>
)
