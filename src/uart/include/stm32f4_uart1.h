/**
 * @file stm32f4_uart1.h
 * @brief Provides serial communcation over UART1.
 *
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef _STM32F4_UART1_H
#define _STM32F4_UART1_H

#include "uart.h"

/**
 * @brief Initialize UART channel 1.
 * Must be called prior to using the channel.
 *
 * @return @ref HAL_STATUS_OK on successful initialization, @ref HAL_STATUS_ERROR otherwise.
 */
hal_status_t stm32f4_uart1_init();

/**
 * @brief Deinitialize UART channel 1.
 *
 * The channel can only be deinitialized if it has previously been initialized.
 *
 * @return @ref HAL_STATUS_OK on successful deinitialization, @ref HAL_STATUS_ERROR otherwise.
 *
 * @note Brings down the peripheral, but not the gpio port that was initialized. This is because
 * gpio ports are shared and unexpected deinits could disrupt other drivers without coordination.
 */
hal_status_t stm32f4_uart1_deinit();

/**
 * @brief Read an incoming byte stream from UART channel 1.
 *
 * Data is placed into the buffer in the same order it was received on
 * the UART channel. There is no way to partition it using this API. Any
 * processing (i.e. parsing for the start of a command) needs to happen at a
 * higher level.
 *
 * @param data A buffer to return read data to client.
 * @param len The max number of bytes to read into buffer. Should not exceed buffer size.
 * @param bytes_read Return the actual number of bytes that were successfully read.
 *
 * @return @ref HAL_STATUS_OK on success, @ref HAL_STATUS_ERROR otherwise.
 */
hal_status_t stm32f4_uart1_read(uint8_t *data, size_t len, size_t *bytes_read);

/**
 * @brief Write an outgoing byte stream to UART channel 1.
 *
 * @param data A buffer filled with data to send.
 * @param len The number of bytes to send. Should not exceed buffer size.
 * @param bytes_written Return the actual number of bytes that were successfully written.
 *
 * @return @ref HAL_STATUS_OK on success, @ref HAL_STATUS_ERROR otherwise.
 */
hal_status_t stm32f4_uart1_write(const uint8_t *data, size_t len, size_t *bytes_written);

#endif /* _STM32F4_UART1_H */
