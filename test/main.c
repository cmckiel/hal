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
	hal_uart_init(NULL);

	while (1)
	{
		for (int i = 0; i < 6; i++)
		{
			hal_gpio_toggle_led();
			hal_delay_ms(100);
		}

		for (int i = 0; i < 6; i++)
		{
			hal_gpio_toggle_led();
			hal_delay_ms(300);
		}
	}

	return 0;
}
