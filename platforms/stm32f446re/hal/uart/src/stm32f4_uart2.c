#include "stm32f4xx.h"
#include "stm32f4_uart2.h"
#include "circular_buffer.h"
#include "gpio.h"

#define SYS_FREQ      16000000     // Default system frequency
#define APB1_CLK      SYS_FREQ
#define UART_BAUDRATE 115200

#define UART_BUFFER_RX_SIZE CIRCULAR_BUFFER_MAX_SIZE
static circular_buffer_ctx rx_ctx;

static void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t periph_clk, uint32_t baud_rate);
static uint16_t compute_uart_bd(uint32_t periph_clk, uint32_t baud_rate);

void USART2_IRQHandler(void)
{
	if (USART2->SR & USART_SR_RXNE)
	{
		uint8_t byte = USART2->DR & 0xFF;
		circular_buffer_push(&rx_ctx, byte);
	}
}

/**
 * @brief Initialize the UART2. (Connected to the USB)
 */
HalStatus_t stm32f4_uart2_init(void *config)
{
	/********************* Buffer Setup *********************/
	// Init the rx buffer.
	if (!circular_buffer_init(&rx_ctx, UART_BUFFER_RX_SIZE))
	{
		return HAL_STATUS_ERROR;
	}

	/********************* GPIO Configure *********************/
	// Enable Bus.
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// Set PA2 mode to alternate function.
	GPIOA->MODER &= ~(1U << 4);
	GPIOA->MODER |= (1U << 5);

	// Set PA3 mode to alternate function.
	GPIOA->MODER &= ~(1U << 6);
	GPIOA->MODER |= (1U << 7);

	// Set PA2 alternate function type to UART_TX (AF07)
	GPIOA->AFR[0] &= ~(0b111100000000UL); // clear bits 11-8
	GPIOA->AFR[0] |= 0b011100000000UL; // set bits 11-8 as 0111 aka AF07 for PA2.

	// Set PA3 alternate function type to UART_RX (AF07)
	GPIOA->AFR[0] &= ~(0b1111000000000000UL); // clear bits 15-12
	GPIOA->AFR[0] |= 0b0111000000000000UL; // set bits 15-12 as 0111 aka AF07 for PA3.

	/********************* UART Configure *********************/
	// Enable the bus.
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

	// Program the M bit in USART_CR1 to define the word length.
	USART2->CR1 &= ~(USART_CR1_M);

	// Select the desired baud rate using the USART_BRR register.
	uart_set_baudrate(USART2, APB1_CLK, UART_BAUDRATE);

	// Set the TE bit in USART_CR1 to send an idle frame as first transmission.
	USART2->CR1 = USART_CR1_TE; // No OR, sets the UART to a default state.
	USART2->CR1 |= USART_CR1_RE; // Enable receiver bit.

	// Set the CR2 register to a default state.
	USART2->CR2 = 0;

	// Enable the USART by writing the UE bit in USART_CR1 register to 1.
	USART2->CR1 |= USART_CR1_UE;

	// Enable RXNE Interrupt.
	USART2->CR1 |= USART_CR1_RXNEIE;

	// Enable NVIC Interrupt.
	NVIC_EnableIRQ(USART2_IRQn);

	return HAL_STATUS_OK;
}

/**
 * @brief Reads data from UART2 register.
*/
HalStatus_t stm32f4_uart2_read(uint8_t *data, size_t len, uint32_t timeout_ms)
{
	uint8_t byte = 0;

	size_t i = 0;
	while (circular_buffer_pop(&rx_ctx, &byte) && i < len)
	{
		data[i] = byte;
	}

    return HAL_STATUS_OK;
}

/**
 * @brief Writes data to UART2 register.
*/
HalStatus_t stm32f4_uart2_write(const uint8_t *data, size_t len)
{
	for (size_t i = 0; i < len; i++)
	{
		while (!(USART2->SR & USART_SR_TXE)); // Wait for TXE
		// Write the data to send in the USART_DR register (this clears the TXE bit). Repeat this for
		// each data to be transmitted in case of single buffer.
		USART2->DR = data[i];
	}

	// After writing the last data into the USART_DR register, wait until TC=1. This indicates that the
	// transmission of the last frame is complete.
    while (!(USART2->SR & USART_SR_TC)); // Wait for last byte fully sent

    return HAL_STATUS_OK;
}

/*
 * @brief Sets the UART baudrate.
 */
static void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t periph_clk, uint32_t baud_rate)
{
	USARTx->BRR = compute_uart_bd(periph_clk, baud_rate);
}

/**
 * @brief Computes the BRR baudrate.
 */
static uint16_t compute_uart_bd(uint32_t periph_clk, uint32_t baud_rate)
{
	return ((periph_clk + (baud_rate/2U)) / baud_rate);
}
