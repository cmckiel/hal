/**
 * @file stm32f4_systick.c
 * @brief STM32F4 SysTick driver: tick counter, blocking delay, and software timers.
 *
 * Copyright (c) 2025 - 2026 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifdef DESKTOP_BUILD
#include "registers.h"
#else
#include "stm32f4xx.h"
#endif
#include <stdbool.h>
#include "hal/systick.h"
#include "stm32f4_hal.h"

#define SYSTICK_LOAD_VAL (16000 - 1U)
#define CTRL_ENABLE      (1U << 0)
#define CTRL_TICKINT     (1U << 1)
#define CTRL_CLKSRC      (1U << 2)

typedef struct {
    hal_timer_callback_t callback;
    uint32_t period_ms;
    uint32_t remaining_ms;
    hal_timer_type_t type;
    bool active;
} software_timer_t;

#ifdef DESKTOP_BUILD
volatile uint32_t tick_ms;
#else
static volatile uint32_t tick_ms;
#endif
static software_timer_t timer_table[HAL_TIMER_MAX_CLIENTS];

void SysTick_Handler(void)
{
    tick_ms++;

    for (int i = 0; i < HAL_TIMER_MAX_CLIENTS; i++)
    {
        if (!timer_table[i].active)
        {
            continue;
        }

        if (--timer_table[i].remaining_ms == 0)
        {
            timer_table[i].callback();

            if (timer_table[i].type == HAL_TIMER_PERIODIC)
            {
                timer_table[i].remaining_ms = timer_table[i].period_ms;
            }
            else
            {
                timer_table[i].active = false;
            }
        }
    }
}

hal_status_t hal_systick_init(void)
{
    SysTick->LOAD = SYSTICK_LOAD_VAL;
    SysTick->VAL  = 0;
    SysTick->CTRL = CTRL_ENABLE | CTRL_CLKSRC | CTRL_TICKINT;
    return HAL_STATUS_OK;
}

uint32_t hal_get_tick(void)
{
    return tick_ms;
}

void hal_delay_ms(uint32_t delay_ms)
{
    uint32_t start = tick_ms;
    while ((tick_ms - start) < delay_ms);
}

hal_timer_handle_t hal_systick_timer_register(uint32_t period_ms, hal_timer_type_t type,
                                      hal_timer_callback_t callback)
{
    if (period_ms == 0 || callback == NULL)
    {
        return HAL_TIMER_INVALID_HANDLE;
    }

    CRITICAL_SECTION_ENTER();

    for (int i = 0; i < HAL_TIMER_MAX_CLIENTS; i++)
    {
        if (!timer_table[i].active)
        {
            timer_table[i].callback     = callback;
            timer_table[i].period_ms    = period_ms;
            timer_table[i].remaining_ms = period_ms;
            timer_table[i].type         = type;
            timer_table[i].active       = true;
            CRITICAL_SECTION_EXIT();
            return (hal_timer_handle_t)i;
        }
    }

    CRITICAL_SECTION_EXIT();
    return HAL_TIMER_INVALID_HANDLE;
}

void hal_systick_timer_deregister(hal_timer_handle_t handle)
{
    if (handle < 0 || handle >= HAL_TIMER_MAX_CLIENTS)
    {
        return;
    }

    CRITICAL_SECTION_ENTER();
    timer_table[handle].active = false;
    CRITICAL_SECTION_EXIT();
}

#ifdef DESKTOP_BUILD
void hal_systick_reset_for_test(void)
{
    tick_ms = 0;
    for (int i = 0; i < HAL_TIMER_MAX_CLIENTS; i++)
    {
        timer_table[i].active = false;
    }
}
#endif
