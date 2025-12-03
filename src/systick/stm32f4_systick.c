/**
 * @file stm32f4_systick.c
 * @brief STM32F4 implementation of millisecond delay.
 *
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifdef DESKTOP_BUILD
#include "registers.h"
#else
#include "stm32f4xx.h"
#endif
#include "systick.h"

#define SYSTICK_LOAD_VAL 16000
#define CTRL_ENABLE      (1U << 0)
#define CTRL_CLKSRC      (1U << 2)
#define CTRL_COUNTFLAG   (1U << 16)

void hal_delay_ms(uint32_t delay)
{
    /* Configure systick */
    /* Reload with number of clocks per millisecond */
    SysTick->LOAD = SYSTICK_LOAD_VAL;

    /* Clear systick current value register */
    SysTick->VAL = 0;

    /* Enable systick and select internal clock source */
    SysTick->CTRL = CTRL_ENABLE | CTRL_CLKSRC;

    for (int i = 0; i < delay; i++)
    {
        /* Wait until the COUNTFLAG is set */
        while ((SysTick->CTRL & CTRL_COUNTFLAG) == 0);

    }

    SysTick->CTRL = 0;
}
