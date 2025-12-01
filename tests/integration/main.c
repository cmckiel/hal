#include <string.h>
#include "systick.h"
#include "uart.h"

#define MAX_RX_BYTES 1024

/**
 * @brief Supports External Loopback Testing by echoing everything received back to sender.
 */
int main(void)
{
	size_t bytes_read_uart1 = 0;
	size_t bytes_read_uart2 = 0;

	size_t bytes_written_uart1 = 0;
	size_t bytes_written_uart2 = 0;

	uint8_t rx_data_uart1[MAX_RX_BYTES] = { 0 };
	uint8_t rx_data_uart2[MAX_RX_BYTES] = { 0 };

	// Initialize drivers.
	hal_uart_init(HAL_UART1);
	hal_uart_init(HAL_UART2);

	while (1)
	{
		hal_delay_ms(10);

		// Reset all data structures.
		bytes_read_uart1 = 0;
		bytes_read_uart2 = 0;

		bytes_written_uart1 = 0;
		bytes_written_uart2 = 0;

		memset(rx_data_uart1, 0, sizeof(rx_data_uart1));
		memset(rx_data_uart2, 0, sizeof(rx_data_uart2));

		// Read incoming data.
		hal_uart_read(HAL_UART1, &rx_data_uart1[0], sizeof(rx_data_uart1), &bytes_read_uart1, 0);
		hal_uart_read(HAL_UART2, &rx_data_uart2[0], sizeof(rx_data_uart2), &bytes_read_uart2, 0);

		// Echo the data back to sender.
		hal_uart_write(HAL_UART1, &rx_data_uart1[0], bytes_read_uart1, &bytes_written_uart1);
		hal_uart_write(HAL_UART2, &rx_data_uart2[0], bytes_read_uart2, &bytes_written_uart2);
	}

	return 0;
}
