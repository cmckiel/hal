#ifndef _GPIO_H
#define _GPIO_H

#include "hal_types.h"

HalStatus_t hal_gpio_init(void *config);
HalStatus_t hal_gpio_toggle_led();

#endif /* _GPIO_H */
