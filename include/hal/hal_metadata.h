/**
 * @file hal_metadata.h
 * @brief Retrieve metadata baked into the HAL during build time.
 *
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef _HAL_METADATA_H
#define _HAL_METADATA_H

/**
 * @brief Struct contains all build and version metadata of HAL.
 */
typedef struct {
    unsigned major;          /*!< Major version. API level breaking changes. */
    unsigned minor;          /*!< Minor verion. Major feature additions without breaking changes. */
    unsigned patch;          /*!< Patch. Bug fixes and small additions without breaking changes. */
    const char *version_str; /*!< String containing major.minor.path ("x.y.z") */
    const char *git_hash;    /*!< Compact hash of the current commit during build. */
    const char *build_date;  /*!< Date of build. ("year-month-day") */
    unsigned dirty;          /*!< 1 if there are uncommitted changes included in this build. (And therefore unknown changes) 0 otherwise. */
    const char *dirty_str;   /*!< Easily printable string representing dirty/clean status. ("dirty", "clean", "unknown") */
} hal_metadata_t;

/**
 * @brief Retrieve the metadata associated with this HAL build.
 *
 * The data is baked in during build configuration.
 *
 * @return A pointer to a static struct containing the build metadata.
 */
const hal_metadata_t *hal_get_metadata(void);

#endif /* _HAL_METADATA_H */
