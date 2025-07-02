#ifndef _STM32F4_HAL_H
#define _STM32F4_HAL_H

#ifdef SIMULATION_BUILD
#define CRITICAL_SECTION_ENTER()
#define CRITICAL_SECTION_EXIT()
#else
#define CRITICAL_SECTION_ENTER() __disable_irq()
#define CRITICAL_SECTION_EXIT()  __enable_irq()
#endif

#define BIT_0 (1U << 0)
#define BIT_1 (1U << 1)
#define BIT_2 (1U << 2)
#define BIT_3 (1U << 3)
#define BIT_4 (1U << 4)
#define BIT_5 (1U << 5)
#define BIT_6 (1U << 6)
#define BIT_7 (1U << 7)
#define BIT_8 (1U << 8)
#define BIT_9 (1U << 9)
#define BIT_10 (1U << 10)
#define BIT_11 (1U << 11)
#define BIT_12 (1U << 12)
#define BIT_13 (1U << 13)
#define BIT_14 (1U << 14)
#define BIT_15 (1U << 15)

#endif /* _STM32F4_HAL_H */
