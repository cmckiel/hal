#ifndef _UART_H
#define _UART_H

#include "hal_types.h"

typedef enum {
    HAL_UART1,
    HAL_UART2,
    HAL_UART3,
} HalUart_t;

int __io_putchar(int ch);

HalStatus_t hal_uart_init(HalUart_t uart, void *config);
HalStatus_t hal_uart_read(HalUart_t uart, uint8_t *data, size_t len, uint32_t timeout_ms);
HalStatus_t hal_uart_write(HalUart_t uart, const uint8_t *data, size_t len);

#endif /* _UART_H */
