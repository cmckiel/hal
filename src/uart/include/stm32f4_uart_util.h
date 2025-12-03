/**
 * @file stm32f4_uart_util.h
 * @brief Provides common calculations for UART channels.
 *
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef _STM32F4_UART_UTIL_H
#define _STM32F4_UART_UTIL_H

#include <stdint.h>

#define SYS_FREQ      16000000     // Default system frequency
#define APB1_CLK      SYS_FREQ
#define APB2_CLK      SYS_FREQ

/**
 * @brief Calculate the 16 bit register value for a desired baud rate.
 *
 * The UART channels each have a baud rate register that determines the rate.
 * (USARTX->BRR). This function calculates the correct value given the clock
 * to the peripheral and the desired baud rate.
 *
 * @param periph_clk The frequency the peripheral is clocked at in Hz.
 * @param baud_rate The desired baud rate. Some common values: 9600, 19200,
 * 38400, 57600, 115200.
 *
 * @return The value to place directly into the Baud Rate Register (BRR).
 */
uint16_t stm32f4_hal_compute_uart_bd(uint32_t periph_clk, uint32_t baud_rate);

#endif /* _STM32F4_UART_UTIL_H */
