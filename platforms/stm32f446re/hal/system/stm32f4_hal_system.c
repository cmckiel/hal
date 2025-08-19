#include "hal_system.h"
#include "stm32f4xx.h"

// Coprocessor full access enables full use of the Floating Point Unit (FPU).
#define CP_FULL_ACCESS 3UL

// Bit locations for coprocessors 10 and 11 in the CPACR register of the
// System Control Block (SCB).
#define CP10 20
#define CP11 22

void hal_system_init()
{
    SystemInit();
}

void SystemInit(void)
{
    // Give CP10 & CP11 full access (FPU)
    SCB->CPACR |= (CP_FULL_ACCESS << CP10) | (CP_FULL_ACCESS << CP11);
    __DSB(); __ISB();  // complete prior writes & flush pipeline
}
