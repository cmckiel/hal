#include "gtest/gtest.h"

extern "C" {
#include "uart.h"
#include "registers.h"
#include "nvic.h"
#include "circular_buffer.h"
}

extern "C" void USART1_IRQHandler(void);
extern "C" void USART2_IRQHandler(void);

class UartDriverTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear peripheral state before each test
        Sim_USART1 = {0};
        Sim_USART2 = {0};
        Sim_GPIOA = {0};
        Sim_RCC = {0};
    }
};

TEST_F(UartDriverTest, InitializesCorrectly)
{
    ASSERT_EQ(hal_uart_init((HalUart_t)(-1), nullptr), HAL_STATUS_ERROR);
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_init(HAL_UART3, nullptr), HAL_STATUS_ERROR);
    ASSERT_EQ(hal_uart_init((HalUart_t)(15), nullptr), HAL_STATUS_ERROR);
}

TEST_F(UartDriverTest, ReadFailsForNullData)
{
    size_t bytes_read = 0;
    size_t data_len = 10; // random
    uint32_t timeout_ms = 0;

    // Uart1 test
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_read(HAL_UART1, nullptr, data_len, &bytes_read, timeout_ms), HAL_STATUS_ERROR);

    // Uart2 test
    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_read(HAL_UART2, nullptr, data_len, &bytes_read, timeout_ms), HAL_STATUS_ERROR);
}

TEST_F(UartDriverTest, ReadFailsForNullBytesRead)
{
    uint8_t data = 0xC8;
    size_t data_len = 1;
    uint32_t timeout_ms = 0;

    // Uart1 test
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_read(HAL_UART1, &data, data_len, nullptr, timeout_ms), HAL_STATUS_ERROR);

    // Uart2 test
    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_read(HAL_UART2, &data, data_len, nullptr, timeout_ms), HAL_STATUS_ERROR);
}

TEST_F(UartDriverTest, ReadSuccessfullyReadsAByte)
{
    uint8_t data = 0;
    size_t data_len = 1;
    size_t bytes_read = 0;
    uint32_t timeout_ms = 0;

    // Uart1 test
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    Sim_USART1.DR = 'A';
    Sim_USART1.SR |= USART_SR_RXNE;
    USART1_IRQHandler();  // simulate interrupt

    ASSERT_EQ(hal_uart_read(HAL_UART1, &data, data_len, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(data, 'A');

    // Uart2 test
    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);

    Sim_USART2.DR = 'A';
    Sim_USART2.SR |= USART_SR_RXNE;
    USART2_IRQHandler();  // simulate interrupt

    ASSERT_EQ(hal_uart_read(HAL_UART2, &data, data_len, &bytes_read, timeout_ms), HAL_STATUS_OK);
}
