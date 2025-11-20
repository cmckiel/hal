#ifndef _HAL_TYPES_H
#define _HAL_TYPES_H

#include <stdint.h>
#include <stdlib.h>

typedef enum {
    HAL_STATUS_OK,
    HAL_STATUS_ERROR,
    HAL_STATUS_BUSY,
    HAL_STATUS_TIMEOUT,
} HalStatus_t;

#endif /* _HAL_TYPES_H */
