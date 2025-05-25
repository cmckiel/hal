#include "stm32f4xx.h"

#define GPIOAEN (1U << 0)
#define PIN5    (1U << 5)
#define LED_PIN PIN5

/**
 * @brief Initializes the LED GPIO to OUTPUT.
 */
void gpio_init(void)
{
	// Enable the peripheral bus clock.
	RCC->AHB1ENR |= GPIOAEN;

	// Configure the GPIO.
	GPIOA->MODER &=~ (1U << 11);
	GPIOA->MODER |= (1U << 10);
}

/**
 * @brief Toggles @ref LED_PIN.
 */
void toggle_led(void)
{
	GPIOA->ODR ^= LED_PIN;
}

