#ifndef _STM32F4_UART2_H
#define _STM32F4_UART2_H

#include "uart.h"

hal_status_t stm32f4_uart2_init();
hal_status_t stm32f4_uart2_deinit();
hal_status_t stm32f4_uart2_read(uint8_t *data, size_t len, size_t *bytes_read, uint32_t timeout_ms);
hal_status_t stm32f4_uart2_write(const uint8_t *data, size_t len, size_t *bytes_written);

#endif /* _STM32F4_UART2_H */
