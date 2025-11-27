/**
 * @file hal_system.h
 * @brief Handles target hardware initialization tasks prior
 * to main application loop.
 *
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef _HAL_SYSTEM_H
#define _HAL_SYSTEM_H

/**
 * @brief Initialize target hardware.
 *
 * Examples could include initializing floating-point coprocessors (FPU),
 * clocks, power, or watchdogs. The intention is that this function gets called
 * prior to any peripheral initialization or application code.
 */
void hal_system_init();

#endif /* _HAL_SYSTEM_H */
