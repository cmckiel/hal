#ifdef DESKTOP_BUILD
#include "registers.h"
#else
#include "stm32f4xx.h"
#endif
#include "gpio.h"

#define GPIOAEN (1U << 0)
#define PIN5    (1U << 5)
#define LED_PIN PIN5

/**
 * @brief Initializes the LED GPIO to OUTPUT.
 */
HalStatus_t hal_gpio_init(void *config)
{
	// Enable the peripheral bus clock.
	RCC->AHB1ENR |= GPIOAEN;

	// Configure the GPIO.
	GPIOA->MODER &=~ (1U << 11);
	GPIOA->MODER |= (1U << 10);

	return HAL_STATUS_OK;
}

/**
 * @brief Toggles @ref LED_PIN.
 */
HalStatus_t hal_gpio_toggle_led()
{
	GPIOA->ODR ^= LED_PIN;
	return HAL_STATUS_OK;
}
