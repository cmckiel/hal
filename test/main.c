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
	uint8_t rx_data[MAX_RX_BYTES] = { 0 };

	hal_gpio_init(NULL);
	hal_uart_init(HAL_UART2, NULL);

	while (1)
	{
		hal_delay_ms(100);

		hal_uart_read(HAL_UART2, &rx_data[0], sizeof(rx_data), 0);

		hal_uart_write(HAL_UART2, &rx_data[0], sizeof(rx_data));
	}

	return 0;
}
