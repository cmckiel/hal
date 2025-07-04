#include "gtest/gtest.h"

extern "C" {
#include "uart.h"
#include "registers.h"
#include "nvic.h"
#include "circular_buffer.h"
}

extern "C" void USART1_IRQHandler(void);
extern "C" void USART2_IRQHandler(void);

// Helper for random test data.
uint8_t random_uint8() {
    return (uint8_t)(rand() % 256);
}

class UartDriverTest : public ::testing::Test {
private:
    static bool seed_is_set;
    int best_seed_ever = 42;
protected:
    void SetUp() override {
        // Clear peripheral state before each test
        Sim_USART1 = {0};
        Sim_USART2 = {0};
        Sim_GPIOA = {0};
        Sim_RCC = {0};

        // Set up the seed only once per full testing run.
        if (!seed_is_set) {
            srand(best_seed_ever);
            seed_is_set = true;
        }
    }
};

bool UartDriverTest::seed_is_set = false;

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

    /********** Uart1 test *********/
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_read(HAL_UART1, &data, data_len, nullptr, timeout_ms), HAL_STATUS_ERROR);

    /********** Uart2 test *********/
    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_read(HAL_UART2, &data, data_len, nullptr, timeout_ms), HAL_STATUS_ERROR);
}

TEST_F(UartDriverTest, ReadsAByte)
{
    uint8_t data = 0;
    size_t data_len = 1;
    size_t bytes_read = 0;
    uint32_t timeout_ms = 0;

    /********** Uart1 test *********/
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    Sim_USART1.DR = 'A';
    Sim_USART1.SR |= USART_SR_RXNE;
    USART1_IRQHandler();  // simulate interrupt

    ASSERT_EQ(hal_uart_read(HAL_UART1, &data, data_len, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(data, 'A');

    /********** Uart2 test *********/
    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);

    Sim_USART2.DR = 'A';
    Sim_USART2.SR |= USART_SR_RXNE;
    USART2_IRQHandler();  // simulate interrupt

    ASSERT_EQ(hal_uart_read(HAL_UART2, &data, data_len, &bytes_read, timeout_ms), HAL_STATUS_OK);
}

TEST_F(UartDriverTest, Uart1ReadsMultipleBytes)
{
    const size_t DATA_LEN = 100;
    uint8_t data_received[DATA_LEN] = {0};
    uint8_t data_read[DATA_LEN] = {0};
    size_t bytes_read = 0;
    uint32_t timeout_ms = 0;

    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    // Receive 100 random bytes from incoming uart.
    for (int i = 0; i < DATA_LEN; i++)
    {
        data_received[i] = random_uint8();
        Sim_USART1.DR = data_received[i];
        Sim_USART1.SR |= USART_SR_RXNE;
        USART1_IRQHandler();  // simulate interrupt
    }

    ASSERT_EQ(hal_uart_read(HAL_UART1, &data_read[0], DATA_LEN, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(DATA_LEN, bytes_read);

    for (int i = 0; i < DATA_LEN; i++)
    {
        ASSERT_EQ(data_received[i], data_read[i]);
    }
}

TEST_F(UartDriverTest, Uart2ReadsMultipleBytes)
{
    const size_t DATA_LEN = 100;
    uint8_t data_received[DATA_LEN] = {0};
    uint8_t data_read[DATA_LEN] = {0};
    size_t bytes_read = 0;
    uint32_t timeout_ms = 0;

    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);

    // Receive 100 random bytes from incoming uart.
    for (int i = 0; i < DATA_LEN; i++)
    {
        data_received[i] = random_uint8();
        Sim_USART2.DR = data_received[i];
        Sim_USART2.SR |= USART_SR_RXNE;
        USART2_IRQHandler();  // simulate interrupt
    }

    ASSERT_EQ(hal_uart_read(HAL_UART2, &data_read[0], DATA_LEN, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(DATA_LEN, bytes_read);

    for (int i = 0; i < DATA_LEN; i++)
    {
        ASSERT_EQ(data_received[i], data_read[i]);
    }
}
