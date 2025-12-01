/**
 * @file uart.h
 * @brief Provides serial communcation over UART1 and UART2.
 *
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef _UART_H
#define _UART_H

#include "hal_types.h"

/**
 * @brief Defines the two available UART channels.
 */
typedef enum {
    HAL_UART1,
    HAL_UART2,
} HalUart_t;

/**
 * @brief syscall declaration for putchar so that printf may be used.
 *
 * @param ch The character to print.
 *
 * @return ch, the value that was passed in as parameter.
 */
int __io_putchar(int ch);

/**
 * @brief
 */
hal_status_t hal_uart_init(HalUart_t uart);

/**
 * @brief
 */
hal_status_t hal_uart_deinit(HalUart_t uart);

/**
 * @brief
 */
hal_status_t hal_uart_read(HalUart_t uart, uint8_t *data, size_t len, size_t *bytes_read, uint32_t timeout_ms);

/**
 * @brief
 */
hal_status_t hal_uart_write(HalUart_t uart, const uint8_t *data, size_t len, size_t *bytes_written);

#endif /* _UART_H */
