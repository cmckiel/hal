// hal_metadata.c
#include "hal_metadata.h"
#include "hal_version.h"

static const hal_metadata_t META = {
    HAL_VERSION_MAJOR,
    HAL_VERSION_MINOR,
    HAL_VERSION_PATCH,
    HAL_VERSION_STR,
    HAL_GIT_HASH,
    HAL_BUILD_DATE
};

const hal_metadata_t *hal_get_metadata(void) {
    return &META;
}
