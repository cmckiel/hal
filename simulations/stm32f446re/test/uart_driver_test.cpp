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

TEST_F(UartDriverTest, Uart1ReadsMaxBytes)
{
    const size_t DATA_LEN = CIRCULAR_BUFFER_MAX_SIZE;
    uint8_t data_received[DATA_LEN] = {0};
    uint8_t data_read[DATA_LEN] = {0};
    size_t bytes_read = 0;
    uint32_t timeout_ms = 0;

    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    // Receive random bytes from incoming uart.
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

TEST_F(UartDriverTest, Uart2ReadsMaxBytes)
{
    const size_t DATA_LEN = CIRCULAR_BUFFER_MAX_SIZE;
    uint8_t data_received[DATA_LEN] = {0};
    uint8_t data_read[DATA_LEN] = {0};
    size_t bytes_read = 0;
    uint32_t timeout_ms = 0;

    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);

    // Receive random bytes from incoming uart.
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

TEST_F(UartDriverTest, Uart1HandlesRXOverflow)
{
    const size_t OVERFLOW_COUNT = 30;
    const size_t DATA_LEN = CIRCULAR_BUFFER_MAX_SIZE + OVERFLOW_COUNT;
    uint8_t data_received[DATA_LEN] = {0};
    uint8_t data_read[DATA_LEN] = {0};
    size_t bytes_read = 0;
    uint32_t timeout_ms = 0;

    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    // Receive random bytes from incoming uart.
    for (int i = 0; i < DATA_LEN; i++)
    {
        data_received[i] = random_uint8();
        Sim_USART1.DR = data_received[i];
        Sim_USART1.SR |= USART_SR_RXNE;
        USART1_IRQHandler();  // simulate interrupt
    }

    ASSERT_EQ(hal_uart_read(HAL_UART1, &data_read[0], DATA_LEN, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(DATA_LEN - OVERFLOW_COUNT, bytes_read);

    // Offset data received by overflow count, since the first 0 -> OVERFLOW_COUNT bytes were overwritten.
    // Only the most recent bytes remain.
    for (int i = 0; i < (DATA_LEN - OVERFLOW_COUNT); i++)
    {
        ASSERT_EQ(data_received[i + OVERFLOW_COUNT], data_read[i]);
    }
}

TEST_F(UartDriverTest, Uart2HandlesRXOverflow)
{
    const size_t OVERFLOW_COUNT = 30;
    const size_t DATA_LEN = CIRCULAR_BUFFER_MAX_SIZE + OVERFLOW_COUNT;
    uint8_t data_received[DATA_LEN] = {0};
    uint8_t data_read[DATA_LEN] = {0};
    size_t bytes_read = 0;
    uint32_t timeout_ms = 0;

    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);

    // Receive random bytes from incoming uart.
    for (int i = 0; i < DATA_LEN; i++)
    {
        data_received[i] = random_uint8();
        Sim_USART2.DR = data_received[i];
        Sim_USART2.SR |= USART_SR_RXNE;
        USART2_IRQHandler();  // simulate interrupt
    }

    ASSERT_EQ(hal_uart_read(HAL_UART2, &data_read[0], DATA_LEN, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(DATA_LEN - OVERFLOW_COUNT, bytes_read);

    // Offset data received by overflow count, since the first 0 -> OVERFLOW_COUNT bytes were overwritten.
    // Only the most recent bytes remain.
    for (int i = 0; i < (DATA_LEN - OVERFLOW_COUNT); i++)
    {
        ASSERT_EQ(data_received[i + OVERFLOW_COUNT], data_read[i]);
    }
}

TEST_F(UartDriverTest, Uart1DoesNotReadPastDataLen)
{
    const size_t ARTIFICIALLY_SMALL_DATA_LEN = 16;

    const size_t DATA_LEN = 64;
    uint8_t data[DATA_LEN] = {0};
    size_t bytes_read = 0;
    uint32_t timeout_ms = 0;

    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    // Receive more bytes from incoming uart than we intend to read.
    for (int i = 0; i < DATA_LEN; i++)
    {
        Sim_USART1.DR = 0xFF;
        Sim_USART1.SR |= USART_SR_RXNE;
        USART1_IRQHandler();  // simulate interrupt
    }

    ASSERT_EQ(hal_uart_read(HAL_UART1, &data[0], ARTIFICIALLY_SMALL_DATA_LEN, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(ARTIFICIALLY_SMALL_DATA_LEN, bytes_read); // We only read up to our provided data len.
    ASSERT_EQ(data[ARTIFICIALLY_SMALL_DATA_LEN], 0); // The next byte in our buffer is untouched.
}

TEST_F(UartDriverTest, Uart2DoesNotReadPastDataLen)
{
    const size_t ARTIFICIALLY_SMALL_DATA_LEN = 16;

    const size_t DATA_LEN = 64;
    uint8_t data[DATA_LEN] = {0};
    size_t bytes_read = 0;
    uint32_t timeout_ms = 0;

    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);

    // Receive more bytes from incoming uart than we intend to read.
    for (int i = 0; i < DATA_LEN; i++)
    {
        Sim_USART2.DR = 0xFF;
        Sim_USART2.SR |= USART_SR_RXNE;
        USART2_IRQHandler();  // simulate interrupt
    }

    ASSERT_EQ(hal_uart_read(HAL_UART2, &data[0], ARTIFICIALLY_SMALL_DATA_LEN, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(ARTIFICIALLY_SMALL_DATA_LEN, bytes_read); // We only read up to our provided data len.
    ASSERT_EQ(data[ARTIFICIALLY_SMALL_DATA_LEN], 0); // The next byte in our buffer is untouched.
}

TEST_F(UartDriverTest, ReadOnEmptyReturnsZeroBytesRead)
{
    const size_t DATA_LEN = 64;
    uint8_t data[DATA_LEN] = {0};
    size_t bytes_read = 0;
    uint32_t timeout_ms = 0;

    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);

    ASSERT_EQ(hal_uart_read(HAL_UART1, &data[0], DATA_LEN, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(bytes_read, 0);

    ASSERT_EQ(hal_uart_read(HAL_UART2, &data[0], DATA_LEN, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(bytes_read, 0);
}
