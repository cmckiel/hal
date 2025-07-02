#ifdef SIMULATION_BUILD
#include "registers.h"
#include "nvic.h"
#else
#include "stm32f4xx.h"
#endif

#include "uart.h"
#include "circular_buffer.h"
#include "stm32f4_hal.h"
// #include "stm32f4_uart1.h"
// #include "stm32f4_uart2.h"

typedef struct {
    USART_TypeDef *USARTx;
    GPIO_TypeDef *GPIOx;
    circular_buffer_ctx rx_ctx;
    circular_buffer_ctx tx_ctx;
	size_t GPIOxEN;
	size_t USARTxEN;
	size_t USARTx_IRQn;
	size_t rx_buffer_size;
	size_t tx_buffer_size;
} stm32f4_uart_t;

static stm32f4_uart_t uart1 = {
	.USARTx = USART1,
	.GPIOx = GPIOA, // @todo determine which gpio port.
	.rx_ctx = {0},
	.tx_ctx = {0},
	.GPIOxEN = RCC_AHB1ENR_GPIOAEN, // @todo fill in the clock enable for gpio port.
	.USARTxEN = RCC_APB2ENR_USART1EN,
	.USARTx_IRQn = USART1_IRQn,
	.rx_buffer_size = CIRCULAR_BUFFER_MAX_SIZE,
	.tx_buffer_size = CIRCULAR_BUFFER_MAX_SIZE,
};

static stm32f4_uart_t uart2 = {
	.USARTx = USART2,
	.GPIOx = GPIOA,
	.rx_ctx = {0},
	.tx_ctx = {0},
	.GPIOxEN = RCC_AHB1ENR_GPIOAEN,
	.USARTxEN = RCC_APB1ENR_USART2EN,
	.USARTx_IRQn = USART2_IRQn,
	.rx_buffer_size = CIRCULAR_BUFFER_MAX_SIZE,
	.tx_buffer_size = CIRCULAR_BUFFER_MAX_SIZE,
};

HalStatus_t hal_uart_init(HalUart_t uart, void *config)
{
	stm32f4_uart_t stm32f4_uart;
	HalStatus_t hal_status = HAL_STATUS_OK;

	// Select the right uart config.
	switch (uart)
	{
		case HAL_UART1:
			stm32f4_uart = uart1;
			break;
		case HAL_UART2:
			stm32f4_uart = uart2;
			break;
		default:
			hal_status = HAL_STATUS_ERROR;
			break;
	}

	// Init RX Buffer
	if (hal_status == HAL_STATUS_OK && !circular_buffer_init(&stm32f4_uart.rx_ctx, stm32f4_uart.rx_buffer_size))
	{
		hal_status = HAL_STATUS_ERROR;
	}

	// Init TX Buffer
	if (hal_status == HAL_STATUS_OK && !circular_buffer_init(&stm32f4_uart.tx_ctx, stm32f4_uart.tx_buffer_size))
	{
		hal_status = HAL_STATUS_ERROR;
	}

	if (hal_status == HAL_STATUS_OK)
	{
		/********************* GPIO Configure *********************/

		// Enable GPIO Bus
		RCC->AHB1ENR |= stm32f4_uart.GPIOxEN; // @todo the bus is not generic.

		// Set PA2 mode to alternate function.
		stm32f4_uart.GPIOx->MODER &= ~BIT_4; // @todo uuuh, these pin mappings might not be generic..
		stm32f4_uart.GPIOx->MODER |= BIT_5;

		// Set PA3 mode to alternate function.
		stm32f4_uart.GPIOx->MODER &= ~BIT_6;
		stm32f4_uart.GPIOx->MODER |= BIT_7;
	}

	return hal_status;
}

HalStatus_t hal_uart_write(HalUart_t uart, const uint8_t *data, size_t len)
{
	HalStatus_t hal_status = HAL_STATUS_ERROR;
	return hal_status;
}

HalStatus_t hal_uart_read(HalUart_t uart, uint8_t *data, size_t len, size_t *bytes_read, uint32_t timeout_ms)
{
	HalStatus_t hal_status = HAL_STATUS_ERROR;
	return hal_status;
}

#ifdef SIMULATION_BUILD

void _hal_uart_inject_rx_buffer_failure(HalUart_t uart)
{
	switch (uart)
	{
		case HAL_UART1:
			uart1.rx_buffer_size = CIRCULAR_BUFFER_MAX_SIZE + 1;
		break;
		case HAL_UART2:
			uart2.rx_buffer_size = CIRCULAR_BUFFER_MAX_SIZE + 1;
		break;
	}
}

void _hal_uart_inject_tx_buffer_failure(HalUart_t uart)
{
	switch (uart)
	{
		case HAL_UART1:
			uart1.tx_buffer_size = CIRCULAR_BUFFER_MAX_SIZE + 1;
		break;
		case HAL_UART2:
			uart2.tx_buffer_size = CIRCULAR_BUFFER_MAX_SIZE + 1;
		break;
	}
}
#endif

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
