#include "stm32f4xx.h"
#include "stm32f4_uart1.h"

// @todo implement
HalStatus_t stm32f4_uart1_init(void *config)
{
	// Enable the interrupt
	NVIC_EnableIRQ(USART1_IRQn);

	// Enable the clock for GPIOA.
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	return HAL_STATUS_ERROR;
}

// @todo implement
HalStatus_t stm32f4_uart1_write(const uint8_t *data, size_t len)
{
	return HAL_STATUS_ERROR;
}

// @todo implement
HalStatus_t stm32f4_uart1_read(uint8_t *data, size_t len, uint32_t timeout_ms)
{
	return HAL_STATUS_ERROR;
}
