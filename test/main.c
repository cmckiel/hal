#include <stdint.h>
#include <stdio.h>
#include "uart.h"
#include "gpio.h"
#include "systick.h"

#define MAX_RX_BYTES 256

/**
 * @brief Entry point to the application.
 *
 * Implemented as a super loop that toggles led and
 * send a message out over UART to a PC.
 */
int main(void)
{
	size_t bytes_read_uart1 = 0;
	size_t bytes_read_uart2 = 0;
	uint8_t rx_data_uart1[MAX_RX_BYTES] = { 0 };
	uint8_t rx_data_uart2[MAX_RX_BYTES] = { 0 };

	hal_gpio_init(NULL);
	hal_uart_init(HAL_UART1, NULL);
	hal_uart_init(HAL_UART2, NULL);

	while (1)
	{
		hal_delay_ms(100);

		bytes_read_uart1 = 0;
		bytes_read_uart2 = 0;
		hal_uart_read(HAL_UART1, &rx_data_uart1[0], sizeof(rx_data_uart1), &bytes_read_uart1, 0);
		hal_uart_read(HAL_UART2, &rx_data_uart2[0], sizeof(rx_data_uart2), &bytes_read_uart2, 0);

		// Solves a weird newline issue.
		if (rx_data_uart1[bytes_read_uart1-1] == '\r')
		{
			rx_data_uart1[bytes_read_uart1] = '\n';
			bytes_read_uart1 += 1;
		}

		// Solves a weird newline issue.
		if (rx_data_uart2[bytes_read_uart2-1] == '\r')
		{
			rx_data_uart2[bytes_read_uart2] = '\n';
			bytes_read_uart2 += 1;
		}

		hal_uart_write(HAL_UART1, &rx_data_uart2[0], bytes_read_uart2);
		hal_uart_write(HAL_UART2, &rx_data_uart1[0], bytes_read_uart1);
	}

	return 0;
}
