add_library(
    stm32f446re_dev_kit
    src/syscalls.c
    src/sysmem.c
    startup/startup_stm32f446retx.s
)
target_include_directories(stm32f446re_dev_kit PUBLIC ./include)
