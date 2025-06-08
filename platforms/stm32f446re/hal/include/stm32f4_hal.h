#ifndef _STM32F4_HAL_H
#define _STM32F4_HAL_H

#ifdef SIMULATION_BUILD
#define CRITICAL_SECTION_ENTER()
#define CRITICAL_SECTION_EXIT()
#else
#define CRITICAL_SECTION_ENTER() __disable_irq()
#define CRITICAL_SECTION_EXIT()  __enable_irq()
#endif

#endif /* _STM32F4_HAL_H */
