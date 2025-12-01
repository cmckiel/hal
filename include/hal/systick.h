/**
 * @file systick.h
 * @brief Provides millisecond delays.
 *
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef _SYSTICK_H
#define _SYSTICK_H

#include "hal_types.h"

/**
 * @brief Delay for a given number of milliseconds.
 *
 * @param delay_ms The number of milliseconds to delay.
 *
 * @note This will block a single threaded application.
 */
void hal_delay_ms(uint32_t delay_ms);

#endif /* _SYSTICK_H */
