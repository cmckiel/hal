#ifndef _STM32F4_UART_UTIL_H
#define _STM32F4_UART_UTIL_H

#include <stdint.h>

#define SYS_FREQ      16000000     // Default system frequency
#define APB1_CLK      SYS_FREQ
#define APB2_CLK      SYS_FREQ

uint16_t stm32f4_hal_compute_uart_bd(uint32_t periph_clk, uint32_t baud_rate);

#endif /* _STM32F4_UART_UTIL_H */
