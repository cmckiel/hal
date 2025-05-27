#include <stdint.h>
#include <stdio.h>
#include "uart.h"
#include "gpio.h"
#include "systick.h"

/**
 * @brief Entry point to the application.
 *
 * Implemented as a super loop that toggles led and
 * send a message out over UART to a PC.
 */
int main(void)
{
	hal_gpio_init(NULL);
	hal_uart_init(HAL_UART2, NULL);

	while (1)
	{
		for (int i = 0; i < 6; i++)
		{
			hal_gpio_toggle_led();
			hal_delay_ms(100);
		}

		// uint8_t data[] = { 'e' };
		// size_t len = 1;
		// hal_uart_write(HAL_UART2, data, len);

		// __io_putchar('x');
		printf("IAmAlive");
		fflush(stdout);

		// hal_delay_ms(200);

		// for (int i = 0; i < 6; i++)
		// {
		// 	hal_gpio_toggle_led();
		// 	hal_delay_ms(300);
		// }
	}

	return 0;
}
