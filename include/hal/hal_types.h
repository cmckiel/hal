/**
 * @file hal_types.h
 * @brief Common types used across HAL modules.
 *
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef _HAL_TYPES_H
#define _HAL_TYPES_H

#include <stdint.h>
#include <stdlib.h>

/**
 * @brief Return type for HAL functions.
 */
typedef enum {
    HAL_STATUS_OK,      /*!< Operation successful. */
    HAL_STATUS_ERROR,   /*!< Operation resulted in an error. */
    HAL_STATUS_BUSY,    /*!< Operation unable to be performed due to busy resource. */
    HAL_STATUS_TIMEOUT, /*!< Operation did not complete within the allocated timeframe. */
} hal_status_t;

#endif /* _HAL_TYPES_H */
