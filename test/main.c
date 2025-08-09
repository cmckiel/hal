#include <string.h>
#include "systick.h"
#include "uart.h"
#include "i2c.h"

#define MAX_RX_BYTES 1024

/**
 * @brief Supports External Loopback Testing by echoing everyting received back to sender.
 */
int main(void)
{
	size_t bytes_read_uart1 = 0;
	size_t bytes_read_uart2 = 0;

	size_t bytes_written_uart1 = 0;
	size_t bytes_written_uart2 = 0;
	size_t bytes_written_i2c = 0;

	uint8_t rx_data_uart1[MAX_RX_BYTES] = { 0 };
	uint8_t rx_data_uart2[MAX_RX_BYTES] = { 0 };

	uint8_t tx_data_i2c[10] = {0};

	// Initialize drivers.
	hal_uart_init(HAL_UART1, NULL);
	hal_uart_init(HAL_UART2, NULL);
	hal_i2c_init(NULL);

	while (1)
	{
		hal_delay_ms(20);

		// Reset all data structures.
		bytes_read_uart1 = 0;
		bytes_read_uart2 = 0;

		bytes_written_uart1 = 0;
		bytes_written_uart2 = 0;
		bytes_written_i2c = 0;

		memset(rx_data_uart1, 0, sizeof(rx_data_uart1));
		memset(rx_data_uart2, 0, sizeof(rx_data_uart2));

		// Read incoming data.
		hal_uart_read(HAL_UART1, &rx_data_uart1[0], sizeof(rx_data_uart1), &bytes_read_uart1, 0);
		hal_uart_read(HAL_UART2, &rx_data_uart2[0], sizeof(rx_data_uart2), &bytes_read_uart2, 0);

		// Echo the data back to sender.
		hal_uart_write(HAL_UART1, &rx_data_uart1[0], bytes_read_uart1, &bytes_written_uart1);
		hal_uart_write(HAL_UART2, &rx_data_uart2[0], bytes_read_uart2, &bytes_written_uart2);


		hal_i2c_write(0x68, tx_data_i2c, 0, &bytes_written_i2c, 0);
		// hal_i2c_write(0x6C, tx_data_i2c, 0, &bytes_written_i2c, 0);

		hal_i2c_event_servicer();
	}

	return 0;
}
