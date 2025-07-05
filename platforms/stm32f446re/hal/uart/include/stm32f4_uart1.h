#ifndef _STM32F4_UART1_H
#define _STM32F4_UART1_H

#include "uart.h"

HalStatus_t stm32f4_uart1_init(void *config);
HalStatus_t stm32f4_uart1_deinit();
HalStatus_t stm32f4_uart1_read(uint8_t *data, size_t len, size_t *bytes_read, uint32_t timeout_ms);
HalStatus_t stm32f4_uart1_write(const uint8_t *data, size_t len);

#endif /* _STM32F4_UART1_H */
