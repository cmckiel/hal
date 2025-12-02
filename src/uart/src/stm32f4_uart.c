/**
 * @file stm32f4_uart.c
 * @brief Provides serial communcation over UART1 and UART2.
 *
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "uart.h"
#include "stm32f4_uart1.h"
#include "stm32f4_uart2.h"

/**
 * @brief Redefine the putchar() function.
 */
int __io_putchar(int ch)
{
	uint8_t data[1] = { 0 };
	size_t bytes_written = 0;

	data[0] = ch;
	hal_uart_write(HAL_UART1, data, sizeof(data), &bytes_written);

	return ch;
}

hal_status_t hal_uart_init(hal_uart_t uart)
{
	hal_status_t hal_status = HAL_STATUS_ERROR;

	if (uart == HAL_UART1)
	{
		hal_status = stm32f4_uart1_init();
	}
	else if (uart == HAL_UART2)
	{
		hal_status = stm32f4_uart2_init();
	}

	return hal_status;
}

hal_status_t hal_uart_deinit(hal_uart_t uart)
{
	hal_status_t hal_status = HAL_STATUS_ERROR;

	if (uart == HAL_UART1)
	{
		hal_status = stm32f4_uart1_deinit();
	}
	else if (uart == HAL_UART2)
	{
		hal_status = stm32f4_uart2_deinit();
	}

	return hal_status;
}

hal_status_t hal_uart_read(hal_uart_t uart, uint8_t *data, size_t len, size_t *bytes_read)
{
	hal_status_t hal_status = HAL_STATUS_ERROR;

	if (uart == HAL_UART1)
	{
		hal_status = stm32f4_uart1_read(data, len, bytes_read);
	}
	else if (uart == HAL_UART2)
	{
		hal_status = stm32f4_uart2_read(data, len, bytes_read);
	}

	return hal_status;
}

hal_status_t hal_uart_write(hal_uart_t uart, const uint8_t *data, size_t len, size_t *bytes_written)
{
	hal_status_t hal_status = HAL_STATUS_ERROR;

	if (uart == HAL_UART1)
	{
		hal_status = stm32f4_uart1_write(data, len, bytes_written);
	}
	else if (uart == HAL_UART2)
	{
		hal_status = stm32f4_uart2_write(data, len, bytes_written);
	}

	return hal_status;
}
