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

hal_status_t hal_uart_init(HalUart_t uart, void *config)
{
	hal_status_t hal_status = HAL_STATUS_ERROR;

	if (uart == HAL_UART1)
	{
		hal_status = stm32f4_uart1_init(config);
	}
	else if (uart == HAL_UART2)
	{
		hal_status = stm32f4_uart2_init(config);
	}
	else if (uart == HAL_UART3)
	{
		// Not implemented.
	}

	return hal_status;
}

hal_status_t hal_uart_deinit(HalUart_t uart)
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
	else if (uart == HAL_UART3)
	{
		// Not implemented.
	}

	return hal_status;
}

hal_status_t hal_uart_read(HalUart_t uart, uint8_t *data, size_t len, size_t *bytes_read, uint32_t timeout_ms)
{
	hal_status_t hal_status = HAL_STATUS_ERROR;

	if (uart == HAL_UART1)
	{
		hal_status = stm32f4_uart1_read(data, len, bytes_read, timeout_ms);
	}
	else if (uart == HAL_UART2)
	{
		hal_status = stm32f4_uart2_read(data, len, bytes_read, timeout_ms);
	}
	else if (uart == HAL_UART3)
	{
		// Not implemented.
	}

	return hal_status;
}

hal_status_t hal_uart_write(HalUart_t uart, const uint8_t *data, size_t len, size_t *bytes_written)
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
	else if (uart == HAL_UART3)
	{
		// Not implemented.
	}

	return hal_status;
}
