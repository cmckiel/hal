#ifndef _STM32F4_HAL_H
#define _STM32F4_HAL_H

#ifdef DESKTOP_BUILD
#define CRITICAL_SECTION_ENTER()
#define CRITICAL_SECTION_EXIT()
#else
#define CRITICAL_SECTION_ENTER() __disable_irq()
#define CRITICAL_SECTION_EXIT()  __enable_irq()
#endif

#define ENUM_IN_RANGE(x, lowerbound_inclusive, upperbound_exclusive) \
((lowerbound_inclusive) <= (x) && (x) < (upperbound_exclusive))

#define AF7_MASK 7U
#define AF4_MASK 4U
#define AF_SHIFT_WIDTH 4U

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
#define BIT_16 (1U << 16)
#define BIT_17 (1U << 17)
#define BIT_18 (1U << 18)
#define BIT_19 (1U << 19)
#define BIT_20 (1U << 20)
#define BIT_21 (1U << 21)
#define BIT_22 (1U << 22)
#define BIT_23 (1U << 23)
#define BIT_24 (1U << 24)
#define BIT_25 (1U << 25)
#define BIT_26 (1U << 26)
#define BIT_27 (1U << 27)
#define BIT_28 (1U << 28)
#define BIT_29 (1U << 29)
#define BIT_30 (1U << 30)
#define BIT_31 (1U << 31)

#define PIN_0 (0)
#define PIN_1 (1)
#define PIN_2 (2)
#define PIN_3 (3)
#define PIN_4 (4)
#define PIN_5 (5)
#define PIN_6 (6)
#define PIN_7 (7)
#define PIN_8 (8)
#define PIN_9 (9)
#define PIN_10 (10)
#define PIN_11 (11)
#define PIN_12 (12)
#define PIN_13 (13)
#define PIN_14 (14)
#define PIN_15 (15)

#endif /* _STM32F4_HAL_H */
