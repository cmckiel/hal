#ifndef _UART_H
#define _UART_H

#include "hal_types.h"

HalStatus_t hal_uart_init(void *config);
HalStatus_t hal_uart_read(uint8_t *data, size_t len, uint32_t timeout_ms);
HalStatus_t hal_uart_write(const uint8_t *data, size_t len);

#endif /* _UART_H */
