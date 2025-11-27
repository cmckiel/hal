/**
 * @file stm32f4_hal_system.c
 * @brief Handles target hardware initialization tasks prior
 * to main application loop.
 *
 * Initializes the FPU to allow for floating-point calculations.
 *
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "hal_system.h"

#define CP_FULL_ACCESS 3UL /*!< Coprocessor full access enables full use of the Floating Point Unit (FPU). */
#define CP10 20 /*!< Bit location for coprocessor 10 in the CPACR register of the System Control Block (SCB) */
#define CP11 22 /*!< Bit location for coprocessor 11 in the CPACR register of the System Control Block (SCB) */

#ifdef DESKTOP_BUILD
// Mock for SystemInit() during desktop builds.
void SystemInit(void) {}

#else
#include "stm32f4xx.h"

// Real system init brings up FPU on target hardware.
void SystemInit(void)
{
    // Give CP10 & CP11 full access (FPU)
    SCB->CPACR |= (CP_FULL_ACCESS << CP10) | (CP_FULL_ACCESS << CP11);
    __DSB(); __ISB();  // complete prior writes & flush pipeline
}

#endif /* DESKTOP_BUILD */

void hal_system_init()
{
    SystemInit();
}
