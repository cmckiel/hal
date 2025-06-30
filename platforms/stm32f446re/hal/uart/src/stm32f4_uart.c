#ifdef SIMULATION_BUILD
#include "registers.h"
#include "nvic.h"
#else
#include "stm32f4xx.h"
#endif

#include "uart.h"
#include "stm32f4_uart1.h"
#include "stm32f4_uart2.h"
#include "circular_buffer.h"

typedef struct {
    USART_TypeDef *USARTx;
    GPIO_TypeDef *GPIOx;
    circular_buffer_ctx rx_ctx;
    circular_buffer_ctx tx_ctx;
	size_t GPIOxEN;
	size_t USARTxEN;
	size_t USARTx_IRQn;
} stm32f4_uart_t;

static stm32f4_uart_t uart1 = {
	.USARTx = USART1,
	.GPIOx = NULL, // @todo determine which gpio port.
	.rx_ctx = {0},
	.tx_ctx = {0},
	.GPIOxEN = 0, // @todo fill in the clock enable for gpio port.
	.USARTxEN = RCC_APB2ENR_USART1EN,
	.USARTx_IRQn = USART1_IRQn,
};

static stm32f4_uart_t uart2 = {
	.USARTx = USART2,
	.GPIOx = GPIOA,
	.rx_ctx = {0},
	.tx_ctx = {0},
	.GPIOxEN = RCC_AHB1ENR_GPIOAEN,
	.USARTxEN = RCC_APB1ENR_USART2EN,
	.USARTx_IRQn = USART2_IRQn,
};

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

HalStatus_t hal_uart_read(HalUart_t uart, uint8_t *data, size_t len, size_t *bytes_read, uint32_t timeout_ms)
{
	HalStatus_t hal_status = HAL_STATUS_ERROR;

	if (uart == HAL_UART1)
	{
		hal_status = stm32f4_uart1_read(data, len, timeout_ms);
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
