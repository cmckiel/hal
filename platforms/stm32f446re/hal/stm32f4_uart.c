#include "uart.h"
#include "stm32f4xx.h"
#include <stdint.h>
#include <stdio.h>

#define GPIOAEN       (1U << 0)
#define USART2EN      (1UL << 17)
#define CR1_UE        (1UL << 13)  // USART ENABLE
#define CR1_M         (1UL << 12)  // USART Word length
#define CR1_TE        (1UL << 3)   // USART Transmit Enable
#define CR1_RE		  (1UL << 2)   // USART Receive  Enable
#define SYS_FREQ      16000000     // Default system frequency
#define APB1_CLK      SYS_FREQ
#define UART_BAUDRATE 115200
#define SR_TC         (1UL << 6)
#define SR_RXNE       (1UL << 5)

void uart_init(void);
HalStatus_t hal_uart_write(const uint8_t *data, size_t len);
int uart_read(void);
void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t periph_clk, uint32_t baud_rate);
uint16_t compute_uart_bd(uint32_t periph_clk, uint32_t baud_rate);

/**
 * @brief Redefine the putchar() function.
 */
int __io_putchar(int ch)
{
	uint8_t data[] = { 0 };
	data[0] = ch;
	size_t len = 1;

	hal_uart_write(&data[0], len);

	return ch;
}

/**
 * @brief Initialize the UART.
 */
HalStatus_t hal_uart_init(void *config)
{
	/********************* GPIO Configure *********************/
	// Enable Bus.
	RCC->AHB1ENR |= GPIOAEN;

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
	RCC->APB1ENR |= USART2EN;

	// Program the M bit in USART_CR1 to define the word length.
	USART2->CR1 &= ~(CR1_M);

	// Select the desired baud rate using the USART_BRR register.
	uart_set_baudrate(USART2, APB1_CLK, UART_BAUDRATE);

	// Set the TE bit in USART_CR1 to send an idle frame as first transmission.
	USART2->CR1 = CR1_TE; // No OR, sets the UART to a default state.
	USART2->CR1 |= CR1_RE; // Enable receiver bit.

	// Set the CR2 register to a default state.
	USART2->CR2 = 0;

	// Enable the USART by writing the UE bit in USART_CR1 register to 1.
	USART2->CR1 |= CR1_UE;

	return HAL_STATUS_OK;
}

/**
 * @brief Writes data to UART2 register.
*/
HalStatus_t hal_uart_write(const uint8_t *data, size_t len)
{
    // @todo fix this.
    char ch = '0';
    if (len > 0)
    {
        ch = data[0];
    }

	// Write the data to send in the USART_DR register (this clears the TXE bit.). Repeat this for
	// each data to be transmitted in case of single buffer.
	USART2->DR = ch & 0xFF;

	// After writing the last data into the USART_DR register, wait until TC=1. This indicates that the
	// transmission of the last frame is complete.
	while ((USART2->SR & SR_TC) != SR_TC); // Wait for the TC bit high to indicate Transmission Complete.
										   // Note to self, seems like the wrong way to wait for this.

    return HAL_STATUS_OK;
}

/**
 * @brief Reads data from UART2 register.
*/
HalStatus_t hal_uart_read(uint8_t *data, size_t len, uint32_t timeout_ms)
{
	// Wait for this bit to be set to indicate that data is received.
	while ((USART2->SR & SR_RXNE) != SR_RXNE);

	uint32_t reg_data = USART2->DR;
	data[0] = (uint8_t)(reg_data & 0xFF);

    return HAL_STATUS_OK;
}

/**
 * @brief Sets the UART baudrate.
 */
void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t periph_clk, uint32_t baud_rate)
{
	USARTx->BRR = compute_uart_bd(periph_clk, baud_rate);
}

/**
 * @brief Computes the BRR baudrate.
 */
uint16_t compute_uart_bd(uint32_t periph_clk, uint32_t baud_rate)
{
	return ((periph_clk + (baud_rate/2U)) / baud_rate);
}
