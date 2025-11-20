#include "stm32f4_uart_util.h"

/**
 * @brief Computes the BRR baudrate.
 */
uint16_t stm32f4_hal_compute_uart_bd(uint32_t periph_clk, uint32_t baud_rate)
{
	return ((periph_clk + (baud_rate/2U)) / baud_rate);
}
