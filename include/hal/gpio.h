/**
 * @file gpio.h
 * @brief This module toggles the onboard LED.
 *
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef _GPIO_H
#define _GPIO_H

#include "hal_types.h"

/**
 * @brief Initialize the module. Must be called only once prior to using toggle.
 *
 * @return @ref HAL_STATUS_OK on success.
 */
hal_status_t hal_gpio_init();

/**
 * @brief Toggle the onboard LED.
 *
 * @return @ref HAL_STATUS_OK on success.
 */
hal_status_t hal_gpio_toggle_led();

#endif /* _GPIO_H */
