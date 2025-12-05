/**
 * @file hal_metadata.c
 * @brief Retrieve metadata baked into the HAL during build time.
 *
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "hal_metadata.h"
#include "hal_version.h"

static const hal_metadata_t META = {
    HAL_VERSION_MAJOR,
    HAL_VERSION_MINOR,
    HAL_VERSION_PATCH,
    HAL_VERSION_STR,
    HAL_GIT_HASH,
    HAL_BUILD_DATE,
    HAL_GIT_DIRTY,
    HAL_GIT_DIRTY_STR
};

const hal_metadata_t *hal_get_metadata()
{
    return &META;
}
