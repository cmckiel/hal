/**
 * @file stm32f4_gpio.c
 * @brief Implementation for toggling the red LED onboard the
 * stm32f446re development board.
 *
 * @copyright
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifdef DESKTOP_BUILD
#include "registers.h"
#else
#include "stm32f4xx.h"
#endif /* DESKTOP_BUILD */

#include "gpio.h"

#define GPIOAEN (1U << 0) /*!< Bit to enable GPIO port A. */
#define PIN5    (1U << 5) /*!< Pin five for GPIO port A. */
#define LED_PIN    (PIN5) /*!< The onboard LED is wired to GPIO port A, pin 5. */

HalStatus_t hal_gpio_init()
{
	// Enable the peripheral bus clock.
	RCC->AHB1ENR |= GPIOAEN;

	// Configure the GPIO.
	GPIOA->MODER &=~ (1U << 11);
	GPIOA->MODER |= (1U << 10);

	// Return HAL_STATUS_OK unconditionally.
	// This implementation does not support the interface's full error reporting capability.
	return HAL_STATUS_OK;
}

HalStatus_t hal_gpio_toggle_led()
{
	// XOR flips bit each call.
	GPIOA->ODR ^= LED_PIN;

	// Return HAL_STATUS_OK unconditionally.
	// This implementation does not support the interface's full error reporting capability.
	return HAL_STATUS_OK;
}
