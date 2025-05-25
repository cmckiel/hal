#ifndef _UART_H
#define _UART_H

void uart_init(void);
int uart_read(void);
int __io_putchar(int ch);

#endif /* _UART_H */
