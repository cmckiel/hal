/**
 * @file uart.h
 * @brief Provides serial communication over UART1 and UART2.
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
    HAL_UART1, /*!< UART Channel 1 */
    HAL_UART2, /*!< UART Channel 2 */
} hal_uart_t;

/**
 * @brief syscall declaration for putchar so that printf may be used.
 *
 * @param ch The character to print.
 *
 * @return ch, the value that was passed in as parameter.
 */
int __io_putchar(int ch);

/**
 * @brief Initialize the UART channel associated with the parameter `uart`.
 * Must be called prior to using the channel.
 *
 * @param uart The UART channel to initialize.
 *
 * @return @ref HAL_STATUS_OK on successful initialization, @ref HAL_STATUS_ERROR otherwise.
 */
hal_status_t hal_uart_init(hal_uart_t uart);

/**
 * @brief Deinitialize the UART channel associated with the parameter `uart`.
 *
 * Only a channel that has been initialized can be successfully deinitialized.
 *
 * @param uart The UART channel to deinitialize.
 *
 * @return @ref HAL_STATUS_OK on successful deinitialization, @ref HAL_STATUS_ERROR otherwise.
 *
 * @note Brings down the peripheral, but not the gpio port that was initialized. This is because
 * gpio ports are shared and unexpected deinits could disrupt other drivers without coordination.
 */
hal_status_t hal_uart_deinit(hal_uart_t uart);

/**
 * @brief Read an incoming byte stream.
 *
 * Data is placed into the buffer in the same order it was received on
 * the UART channel. There is no way to partition it using this API. Any
 * processing (i.e. parsing for the start of a command) needs to happen at a
 * higher level.
 *
 * @param uart The UART channel to read from.
 * @param data A buffer to return read data to client.
 * @param len The max number of bytes to read into buffer. Should not exceed buffer size.
 * @param bytes_read Return the actual number of bytes that were successfully read.
 *
 * @return @ref HAL_STATUS_OK on success, @ref HAL_STATUS_ERROR otherwise.
 */
hal_status_t hal_uart_read(hal_uart_t uart, uint8_t *data, size_t len, size_t *bytes_read);

/**
 * @brief Write an outgoing byte stream.
 *
 * @param uart The UART channel to write to.
 * @param data A buffer filled with data to send.
 * @param len The number of bytes to send. Should not exceed buffer size.
 * @param bytes_written Return the actual number of bytes that were successfully written.
 *
 * @return @ref HAL_STATUS_OK on success, @ref HAL_STATUS_ERROR otherwise.
 */
hal_status_t hal_uart_write(hal_uart_t uart, const uint8_t *data, size_t len, size_t *bytes_written);

#endif /* _UART_H */
