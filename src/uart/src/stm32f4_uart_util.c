/**
 * @file stm32f4_uart_util.c
 * @brief Provides common calculations for UART channels.
 *
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "stm32f4_uart_util.h"

uint16_t stm32f4_hal_compute_uart_bd(uint32_t periph_clk, uint32_t baud_rate)
{
	// Common rounding trick for integers:
	// result = (numerator * scale + divisor/2) / divisor;
	return ((periph_clk + (baud_rate/2U)) / baud_rate);
}
