#ifdef SIMULATION_BUILD
#include "registers.h"
#include "nvic.h"
#else
#include "stm32f4xx.h"
#endif

#include "stm32f4_hal.h"
#include "stm32f4_uart_util.h"
#include "stm32f4_uart1.h"
#include "circular_buffer.h"

#define UART_BAUDRATE 115200

#define UART_BUFFER_RX_SIZE CIRCULAR_BUFFER_MAX_SIZE
#define UART_BUFFER_TX_SIZE CIRCULAR_BUFFER_MAX_SIZE

static circular_buffer_ctx rx_ctx;
static circular_buffer_ctx tx_ctx;

static void configure_gpio_pins();
static void configure_uart();
static void configure_interrupt();

void USART1_IRQHandler(void)
{
	if (USART1->SR & USART_SR_RXNE)
	{
		// A received byte is waiting in data register.
		uint8_t byte = USART1->DR & 0xFF;
		circular_buffer_push_with_overwrite(&rx_ctx, byte);
	}

	if (USART1->SR & USART_SR_TXE)
	{
		// Transmit register is empty. Ready for a new byte.
		uint8_t byte = 0;
		if (circular_buffer_pop(&tx_ctx, &byte))
		{
			USART1->DR = byte;
		}
		else
		{
			// Buffer empty — stop TXE interrupt to prevent ISR from firing again
			USART1->CR1 &= ~USART_CR1_TXEIE;
		}
	}
}

HalStatus_t stm32f4_uart1_init(void *config)
{
	if (!circular_buffer_init(&rx_ctx, UART_BUFFER_RX_SIZE) ||
		!circular_buffer_init(&tx_ctx, UART_BUFFER_TX_SIZE))
	{
		return HAL_STATUS_ERROR;
	}

	configure_gpio_pins();
	configure_uart();
	configure_interrupt();

	return HAL_STATUS_OK;
}

HalStatus_t stm32f4_uart1_read(uint8_t *data, size_t len, size_t *bytes_read, uint32_t timeout_ms)
{
	HalStatus_t res = HAL_STATUS_ERROR;

	if (data && bytes_read)
	{
		*bytes_read = 0;
		res = HAL_STATUS_OK;

		uint8_t byte = 0;

		// While there are still bytes in the buffer and we still have space to store them.
		while (!circular_buffer_is_empty(&rx_ctx) && *bytes_read < len)
		{
			CRITICAL_SECTION_ENTER();
			bool popped = circular_buffer_pop(&rx_ctx, &byte);
			CRITICAL_SECTION_EXIT();

			if (popped)
			{
				data[*bytes_read] = byte;
				*bytes_read = *bytes_read + 1;
			}
			else
			{
				// Error
				res = HAL_STATUS_ERROR;
				break;
			}
		}
	}

    return res;
}

HalStatus_t stm32f4_uart1_write(const uint8_t *data, size_t len)
{
	HalStatus_t res = HAL_STATUS_ERROR;
	size_t current_buffer_capacity = 0;

	CRITICAL_SECTION_ENTER();

	bool buffer_was_empty = circular_buffer_is_empty(&tx_ctx);

	if (circular_buffer_get_current_capacity(&tx_ctx, &current_buffer_capacity) &&
		0 < len && len <= current_buffer_capacity)
	{
		res = HAL_STATUS_OK;
		// The entirety of the data will fit.
		// Put the data into buffer.
		for (size_t i = 0; i < len; i++)
		{
			if (!circular_buffer_push_no_overwrite(&tx_ctx, data[i]))
			{
				// Something went wrong. We were unable to push despite
				// there being capacity.
				res = HAL_STATUS_ERROR;
				break;
			}
		}

		if (res == HAL_STATUS_OK && buffer_was_empty)
		{
			USART1->CR1 |= USART_CR1_TXEIE;  // Enable TXE interrupt
		}

	}

	CRITICAL_SECTION_EXIT();

	return res;
}

static void configure_gpio_pins()
{
	// Enable Bus.
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// Set PA9 (usart1 tx pin) mode to alternate function.
	GPIOA->MODER &= ~BIT_18;
	GPIOA->MODER |= BIT_19;

	// Set PA10 (usart1 rx pin) mode to alternate function.
	GPIOA->MODER &= ~BIT_20;
	GPIOA->MODER |= BIT_21;

	// Set PA9 alternate function type to UART_TX (AF07)
	// Pin number is misleading here, because AFR is divided into high and low regs.
	GPIOA->AFR[1] &= ~(0xFF << (PIN_1 * AF_SHIFT_WIDTH));
	GPIOA->AFR[1] |=  (AF7_MASK << (PIN_1 * AF_SHIFT_WIDTH));

	// Set PA10 alternate function type to UART_RX (AF07)
	// Pin number is misleading here, because AFR is divided into high and low regs.
	GPIOA->AFR[1] &= ~(0xFF << (PIN_2 * AF_SHIFT_WIDTH)); // clear bits 15-12
	GPIOA->AFR[1] |= (AF7_MASK << (PIN_2 * AF_SHIFT_WIDTH)); // set bits 15-12 as 0111 aka AF07 for PA3.
}

static void configure_uart()
{
	// Enable the bus.
	RCC->APB2ENR |= RCC_APB2ENR_USART1EN;

	// Program the M bit in USART_CR1 to define the word length.
	USART1->CR1 &= ~(USART_CR1_M);

	// Select the desired baud rate using the USART_BRR register.
	USART1->BRR = stm32f4_hal_compute_uart_bd(APB2_CLK, UART_BAUDRATE);

	// Set the TE bit in USART_CR1 to send an idle frame as first transmission.
	USART1->CR1 = USART_CR1_TE; // No OR, sets the UART to a default state.
	USART1->CR1 |= USART_CR1_RE; // Enable receiver bit.

	// Set the CR2 register to a default state.
	USART1->CR2 = 0;

	// Enable the USART by writing the UE bit in USART_CR1 register to 1.
	USART1->CR1 |= USART_CR1_UE;
}

static void configure_interrupt()
{
	// Enable RXNE Interrupt.
	USART1->CR1 |= USART_CR1_RXNEIE;

	// Enable NVIC Interrupt.
	NVIC_EnableIRQ(USART1_IRQn);
}
