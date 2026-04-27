/**
 * @file systick.h
 * @brief SysTick driver: millisecond tick counter, blocking delay, and software timers.
 *
 * Copyright (c) 2025 - 2026 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef _SYSTICK_H
#define _SYSTICK_H

#include <stdint.h>
#include "hal_types.h"

#define HAL_TIMER_INVALID_HANDLE (-1)
#define HAL_TIMER_MAX_CLIENTS    (8)

typedef enum {
    HAL_TIMER_PERIODIC,
    HAL_TIMER_ONE_SHOT,
} hal_timer_type_t;

typedef void (*hal_timer_callback_t)(void);

/**
 * @brief Opaque handle returned by hal_systick_timer_register. Negative value indicates invalid.
 */
typedef int8_t hal_timer_handle_t;

/**
 * @brief Initialize and start the SysTick peripheral at 1 ms resolution.
 *
 * Must be called before hal_delay_ms, hal_get_tick, or hal_systick_timer_register.
 */
hal_status_t hal_systick_init(void);

/**
 * @brief Return milliseconds elapsed since hal_systick_init was called.
 */
uint32_t hal_get_tick(void);

/**
 * @brief Block for the given number of milliseconds.
 *
 * @note Requires hal_systick_init to have been called. Registered timer
 *       callbacks will still fire during the delay.
 */
void hal_delay_ms(uint32_t delay_ms);

/**
 * @brief Register a software timer that fires a callback at a fixed period.
 *
 * @param period_ms  Period (one-shot: delay) in milliseconds. Must be > 0.
 * @param type       HAL_TIMER_PERIODIC or HAL_TIMER_ONE_SHOT.
 * @param callback   Function to call on expiry. Must not be NULL.
 *
 * @return Non-negative handle on success, HAL_TIMER_INVALID_HANDLE if the
 *         timer table is full or arguments are invalid.
 */
hal_timer_handle_t hal_systick_timer_register(uint32_t period_ms, hal_timer_type_t type,
                                      hal_timer_callback_t callback);

/**
 * @brief Deregister a previously registered software timer.
 *
 * @param handle Handle returned by hal_systick_timer_register. No-op for invalid handles.
 */
void hal_systick_timer_deregister(hal_timer_handle_t handle);

#endif /* _SYSTICK_H */
