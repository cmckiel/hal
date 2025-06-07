#ifndef _STM32F4_HAL_H
#define _STM32F4_HAL_H

#define CRITICAL_SECTION_ENTER() __disable_irq()
#define CRITICAL_SECTION_EXIT()  __enable_irq()

#endif /* _STM32F4_HAL_H */
