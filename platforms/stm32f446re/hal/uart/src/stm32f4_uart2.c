#ifdef SIMULATION_BUILD
#include "registers.h"
#include "nvic.h"
#else
#include "stm32f4xx.h"
#endif

#include "stm32f4_hal.h"
#include "stm32f4_uart_util.h"
#include "stm32f4_uart2.h"
#include "circular_buffer.h"

#define UART_BAUDRATE 115200

#define UART_BUFFER_RX_SIZE CIRCULAR_BUFFER_MAX_SIZE
#define UART_BUFFER_TX_SIZE CIRCULAR_BUFFER_MAX_SIZE

static bool uart2_initialized = false;

static circular_buffer_ctx rx_ctx;
static circular_buffer_ctx tx_ctx;

static void configure_gpio_pins();
static void configure_uart();
static void configure_interrupt();

void USART2_IRQHandler(void)
{
	if (USART2->SR & USART_SR_RXNE)
	{
		// A received byte is waiting in data register.
		uint8_t byte = USART2->DR & 0xFF;
		circular_buffer_push_with_overwrite(&rx_ctx, byte);
	}

	if (USART2->SR & USART_SR_TXE)
	{
		// Transmit register is empty. Ready for a new byte.
		uint8_t byte = 0;
		if (circular_buffer_pop(&tx_ctx, &byte))
		{
			USART2->DR = byte;
		}
		else
		{
			// Buffer empty — stop TXE interrupt to prevent ISR from firing again
			USART2->CR1 &= ~USART_CR1_TXEIE;
		}
	}
}

/**
 * @brief Initialize the UART2. (Connected to the USB)
 */
HalStatus_t stm32f4_uart2_init(void *config)
{
	// Prevent multiple initialization
	if (uart2_initialized)
	{
		return HAL_STATUS_ERROR;
	}

	if (!circular_buffer_init(&rx_ctx, UART_BUFFER_RX_SIZE) ||
		!circular_buffer_init(&tx_ctx, UART_BUFFER_TX_SIZE))
	{
		return HAL_STATUS_ERROR;
	}

	configure_gpio_pins();
	configure_uart();
	configure_interrupt();

	uart2_initialized = true; // Mark as initialized only after success.
	return HAL_STATUS_OK;
}

HalStatus_t stm32f4_uart2_deinit()
{
    if (!uart2_initialized)
	{
        return HAL_STATUS_ERROR;
    }

    // Disable interrupts
    USART2->CR1 &= ~(USART_CR1_RXNEIE | USART_CR1_TXEIE);
    NVIC_DisableIRQ(USART2_IRQn);

    // Disable UART
    USART2->CR1 &= ~USART_CR1_UE;

    // Disable clock
	RCC->APB1ENR &= ~RCC_APB1ENR_USART2EN;

    uart2_initialized = false;
    return HAL_STATUS_OK;
}

/**
 * @brief Reads data from UART2 register.
*/
HalStatus_t stm32f4_uart2_read(uint8_t *data, size_t len, size_t *bytes_read, uint32_t timeout_ms)
{
	HalStatus_t res = HAL_STATUS_ERROR;

	if (data && bytes_read && uart2_initialized)
	{
		*bytes_read = 0;
		res = HAL_STATUS_OK;

		uint8_t byte = 0;

		// While there are still bytes in the buffer and we still have space to store them.
		while (!circular_buffer_is_empty(&rx_ctx) && *bytes_read < len)
		{
			// ***************************************************
			// CRITICAL_SECTION_ENTER
			NVIC_DisableIRQ(USART2_IRQn);
			bool popped = circular_buffer_pop(&rx_ctx, &byte);
			NVIC_EnableIRQ(USART2_IRQn);
			// CRITICAL_SECTION_EXIT
			// ***************************************************

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

/**
 * @brief Writes data to UART2 register.
*/
HalStatus_t stm32f4_uart2_write(const uint8_t *data, size_t len, size_t *bytes_written)
{
	HalStatus_t res = HAL_STATUS_ERROR;
	bool push_success = true;

	if (uart2_initialized && bytes_written && data && len > 0)
	{
		*bytes_written = 0;
		for (size_t i = 0; i < len; i++)
		{
			// ***************************************************
			// CRITICAL_SECTION_ENTER
			NVIC_DisableIRQ(USART2_IRQn);
			push_success = circular_buffer_push_no_overwrite(&tx_ctx, data[i]);
			NVIC_EnableIRQ(USART2_IRQn);
			// CRITICAL_SECTION_EXIT
			// ***************************************************

			if (push_success)
			{
				*bytes_written += 1;
			}
			else
			{
				break;
			}
		}

		// If bytes were written successfully to buffer, then enable the transmit
		// interrupt because those bytes need to be sent out.
		if (*bytes_written > 0)
		{
			USART2->CR1 |= USART_CR1_TXEIE;  // Enable TXE interrupt
		}

		// If we successfully wrote all bytes to buffer, then the function was an
		// overall success.
		res = (*bytes_written == len) ? HAL_STATUS_OK : HAL_STATUS_ERROR;
	}

	return res;
}

static void configure_gpio_pins()
{
	// Enable Bus.
	RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;

	// Set PA2 (usart2 tx pin) mode to alternate function.
	GPIOA->MODER &= ~BIT_4;
	GPIOA->MODER |= BIT_5;

	// Set PA3 (usart2 rx pin) mode to alternate function.
	GPIOA->MODER &= ~BIT_6;
	GPIOA->MODER |= BIT_7;

	// Set PA2 alternate function type to UART_TX (AF07)
	GPIOA->AFR[0] &= ~(0xF << (PIN_2 * AF_SHIFT_WIDTH)); // clear bits 11-8
	GPIOA->AFR[0] |=  (AF7_MASK << (PIN_2 * AF_SHIFT_WIDTH)); // set bits 11-8 as 0111 aka AF07 for PA2.

	// Set PA3 alternate function type to UART_RX (AF07)
	GPIOA->AFR[0] &= ~(0xF << (PIN_3 * AF_SHIFT_WIDTH)); // clear bits 15-12
	GPIOA->AFR[0] |= (AF7_MASK << (PIN_3 * AF_SHIFT_WIDTH)); // set bits 15-12 as 0111 aka AF07 for PA3.
}

static void configure_uart()
{
	// Enable the bus.
	RCC->APB1ENR |= RCC_APB1ENR_USART2EN;

	// Program the M bit in USART_CR1 to define the word length.
	USART2->CR1 &= ~(USART_CR1_M);

	// Select the desired baud rate using the USART_BRR register.
	USART2->BRR = stm32f4_hal_compute_uart_bd(APB1_CLK, UART_BAUDRATE);

	// Set the TE bit in USART_CR1 to send an idle frame as first transmission.
	USART2->CR1 = USART_CR1_TE; // No OR, sets the UART to a default state.
	USART2->CR1 |= USART_CR1_RE; // Enable receiver bit.

	// Set the CR2 register to a default state.
	USART2->CR2 = 0;

	// Enable the USART by writing the UE bit in USART_CR1 register to 1.
	USART2->CR1 |= USART_CR1_UE;
}

static void configure_interrupt()
{
	// Enable RXNE Interrupt.
	USART2->CR1 |= USART_CR1_RXNEIE;

	// Enable NVIC Interrupt.
	NVIC_EnableIRQ(USART2_IRQn);
}
