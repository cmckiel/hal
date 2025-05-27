#include "uart.h"
#include "stm32f4_uart1.h"
#include "stm32f4_uart2.h"

/**
 * @brief Redefine the putchar() function.
 */
int __io_putchar(int ch)
{
	uint8_t data[] = { 0 };
	data[0] = ch;
	size_t len = 1;

	hal_uart_write(HAL_UART2, &data[0], len);

	return ch;
}

HalStatus_t hal_uart_init(HalUart_t uart, void *config)
{
	HalStatus_t hal_status = HAL_STATUS_ERROR;

	if (uart == HAL_UART1)
	{
		stm32f4_uart1_init(config);
	}
	else if (uart == HAL_UART2)
	{
		stm32f4_uart2_init(config);
	}
	else if (uart == HAL_UART3)
	{
		// Not implemented.
	}

	return hal_status;
}

HalStatus_t hal_uart_write(HalUart_t uart, const uint8_t *data, size_t len)
{
	HalStatus_t hal_status = HAL_STATUS_ERROR;

	if (uart == HAL_UART1)
	{
		hal_status = stm32f4_uart1_write(data, len);
	}
	else if (uart == HAL_UART2)
	{
		hal_status = stm32f4_uart2_write(data, len);
	}
	else if (uart == HAL_UART3)
	{
		// Not implemented.
	}

	return hal_status;
}

HalStatus_t hal_uart_read(HalUart_t uart, uint8_t *data, size_t len, uint32_t timeout_ms)
{
	HalStatus_t hal_status = HAL_STATUS_ERROR;

	if (uart == HAL_UART1)
	{
		hal_status = stm32f4_uart1_read(data, len, timeout_ms);
	}
	else if (uart == HAL_UART2)
	{
		hal_status = stm32f4_uart2_read(data, len, timeout_ms);
	}
	else if (uart == HAL_UART3)
	{
		// Not implemented.
	}

	return hal_status;
}
