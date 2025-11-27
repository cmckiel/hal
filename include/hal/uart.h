#ifndef _UART_H
#define _UART_H

#include "hal_types.h"

typedef enum {
    HAL_UART1,
    HAL_UART2,
    HAL_UART3,
} HalUart_t;

typedef struct {
    size_t rx_dropped_bytes;
    size_t rx_buffer_capacity;
    size_t rx_buffer_used;
} HalUartStats_t;

int __io_putchar(int ch);

hal_status_t hal_uart_init(HalUart_t uart, void *config);
hal_status_t hal_uart_deinit(HalUart_t uart);
hal_status_t hal_uart_read(HalUart_t uart, uint8_t *data, size_t len, size_t *bytes_read, uint32_t timeout_ms);
hal_status_t hal_uart_write(HalUart_t uart, const uint8_t *data, size_t len, size_t *bytes_written);
hal_status_t hal_uart_get_stats(HalUart_t uart, HalUartStats_t *stats);

#endif /* _UART_H */
