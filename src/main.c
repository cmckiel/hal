#include "stm32f4xx.h"
#include <stdint.h>
#include <stdio.h>
#include "uart.h"
#include "gpio.h"
#include "adc.h"
#include "systick.h"

/**
 * @brief Entry point to the application.
 *
 * Implemented as a super loop that toggles led and
 * send a message out over UART to a PC.
 */
int main(void)
{
	gpio_init();
	uart_init();

	int count = 0;
	while (1)
	{
		toggle_led();
		printf("%d\r\n", count);
		printf("\33[2J"); // Use VT100 escape codes
		printf("\33[H");
		count++;
		SysTickDelayMs(50);
	}

	return 0;
}
