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

    void TearDown() override {
        hal_uart_deinit(HAL_UART1);
        hal_uart_deinit(HAL_UART2);
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

TEST_F(UartDriverTest, ReadWithVeryLargeLengthRequest)
{
    const size_t HUGE_LEN = SIZE_MAX;  // or some very large number
    uint8_t single_byte;
    size_t bytes_read = 0;

    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    // Put one byte in buffer
    Sim_USART1.DR = 'A';
    Sim_USART1.SR |= USART_SR_RXNE;
    USART1_IRQHandler();

    // Request way more than available - should only read what's there
    ASSERT_EQ(hal_uart_read(HAL_UART1, &single_byte, HUGE_LEN, &bytes_read, 0), HAL_STATUS_OK);
    ASSERT_EQ(bytes_read, 1);  // Only got what was available
    ASSERT_EQ(single_byte, 'A');
}

TEST_F(UartDriverTest, ReadWithZeroLengthReturnsOK)
{
    uint8_t data[10] = {0};
    size_t bytes_read = 999; // Initialize to non-zero to verify it gets set
    uint32_t timeout_ms = 0;

    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);

    // Put some data in the buffer
    Sim_USART1.DR = 'A';
    Sim_USART1.SR |= USART_SR_RXNE;
    USART1_IRQHandler();

    Sim_USART2.DR = 'B';
    Sim_USART2.SR |= USART_SR_RXNE;
    USART2_IRQHandler();

    // Request zero bytes - should return OK with bytes_read = 0
    ASSERT_EQ(hal_uart_read(HAL_UART1, data, 0, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(bytes_read, 0);
    ASSERT_EQ(data[0], 0); // Buffer should be untouched

    ASSERT_EQ(hal_uart_read(HAL_UART2, data, 0, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(bytes_read, 0);
}

TEST_F(UartDriverTest, ReadWithInvalidUartEnum)
{
    uint8_t data[10] = {0};
    size_t bytes_read = 0;
    uint32_t timeout_ms = 0;

    // Test various invalid UART enum values
    ASSERT_EQ(hal_uart_read((HalUart_t)(-1), data, sizeof(data), &bytes_read, timeout_ms), HAL_STATUS_ERROR);
    ASSERT_EQ(hal_uart_read(HAL_UART3, data, sizeof(data), &bytes_read, timeout_ms), HAL_STATUS_ERROR); // Not implemented
    ASSERT_EQ(hal_uart_read((HalUart_t)(99), data, sizeof(data), &bytes_read, timeout_ms), HAL_STATUS_ERROR);
    ASSERT_EQ(hal_uart_read((HalUart_t)(SIZE_MAX), data, sizeof(data), &bytes_read, timeout_ms), HAL_STATUS_ERROR);
}

TEST_F(UartDriverTest, Uart1ReadDuringSimulatedInterrupt)
{
    const size_t INITIAL_DATA_LEN = 5;
    const size_t ADDITIONAL_DATA_LEN = 3;
    uint8_t data[INITIAL_DATA_LEN + ADDITIONAL_DATA_LEN] = {0};
    size_t bytes_read = 0;
    uint32_t timeout_ms = 0;

    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    // Put initial data in buffer via simulated interrupts
    for (int i = 0; i < INITIAL_DATA_LEN; i++) {
        Sim_USART1.DR = 'A' + i;  // A, B, C, D, E
        Sim_USART1.SR |= USART_SR_RXNE;
        USART1_IRQHandler();
    }

    // Start reading only part of the data
    ASSERT_EQ(hal_uart_read(HAL_UART1, data, 3, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(bytes_read, 3);
    ASSERT_EQ(data[0], 'A');
    ASSERT_EQ(data[1], 'B');
    ASSERT_EQ(data[2], 'C');

    // Simulate more data arriving via interrupt while buffer still has unread data
    for (int i = 0; i < ADDITIONAL_DATA_LEN; i++) {
        Sim_USART1.DR = 'X' + i;  // X, Y, Z
        Sim_USART1.SR |= USART_SR_RXNE;
        USART1_IRQHandler();
    }

    // Read the remaining data - should get original remainder + new data
    ASSERT_EQ(hal_uart_read(HAL_UART1, &data[3], 5, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(bytes_read, 5);  // D, E, X, Y, Z
    ASSERT_EQ(data[3], 'D');   // Remaining original data
    ASSERT_EQ(data[4], 'E');
    ASSERT_EQ(data[5], 'X');   // New data
    ASSERT_EQ(data[6], 'Y');
    ASSERT_EQ(data[7], 'Z');
}

TEST_F(UartDriverTest, Uart2BufferStateConsistency)
{
    const size_t HALF_BUFFER = CIRCULAR_BUFFER_MAX_SIZE / 2;
    uint8_t data[CIRCULAR_BUFFER_MAX_SIZE] = {0};
    size_t bytes_read = 0;
    uint32_t timeout_ms = 0;

    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);

    // Fill buffer to half capacity with known pattern
    for (int i = 0; i < HALF_BUFFER; i++) {
        Sim_USART2.DR = (uint8_t)(i & 0xFF);
        Sim_USART2.SR |= USART_SR_RXNE;
        USART2_IRQHandler();
    }

    // Read a quarter of the data
    size_t quarter = HALF_BUFFER / 2;
    ASSERT_EQ(hal_uart_read(HAL_UART2, data, quarter, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(bytes_read, quarter);

    // Verify we got the expected data (first quarter of pattern)
    for (int i = 0; i < quarter; i++) {
        ASSERT_EQ(data[i], (uint8_t)(i & 0xFF));
    }

    // Add more data to test buffer wrap-around behavior
    for (int i = HALF_BUFFER; i < HALF_BUFFER + quarter; i++) {
        Sim_USART2.DR = (uint8_t)(i & 0xFF);
        Sim_USART2.SR |= USART_SR_RXNE;
        USART2_IRQHandler();
    }

    // Read remaining data - should be in correct order
    size_t remaining_expected = (HALF_BUFFER - quarter) + quarter;
    ASSERT_EQ(hal_uart_read(HAL_UART2, &data[quarter], remaining_expected, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(bytes_read, remaining_expected);

    // Verify data integrity - should be sequential from where we left off
    for (int i = 0; i < remaining_expected; i++) {
        ASSERT_EQ(data[quarter + i], (uint8_t)((quarter + i) & 0xFF));
    }

    // Buffer should now be empty
    ASSERT_EQ(hal_uart_read(HAL_UART2, data, 1, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(bytes_read, 0);
}

TEST_F(UartDriverTest, WriteFailsForNullData)
{
    size_t data_len = 10;

    // UART1 test
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_write(HAL_UART1, nullptr, data_len), HAL_STATUS_ERROR);

    // UART2 test
    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_write(HAL_UART2, nullptr, data_len), HAL_STATUS_ERROR);
}

TEST_F(UartDriverTest, WriteFailsForZeroLength)
{
    uint8_t data[] = "Hello";

    // UART1 test
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_write(HAL_UART1, data, 0), HAL_STATUS_ERROR);

    // UART2 test
    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_write(HAL_UART2, data, 0), HAL_STATUS_ERROR);
}

TEST_F(UartDriverTest, WriteFailsForInvalidUart)
{
    uint8_t data[] = "Test";
    size_t data_len = sizeof(data) - 1;

    ASSERT_EQ(hal_uart_write((HalUart_t)(-1), data, data_len), HAL_STATUS_ERROR);
    ASSERT_EQ(hal_uart_write(HAL_UART3, data, data_len), HAL_STATUS_ERROR); // Not implemented
    ASSERT_EQ(hal_uart_write((HalUart_t)(99), data, data_len), HAL_STATUS_ERROR);
}

TEST_F(UartDriverTest, Uart1WritesSingleByte)
{
    uint8_t data[] = {'A'};

    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_write(HAL_UART1, data, 1), HAL_STATUS_OK);

    // Verify TXE interrupt was enabled (buffer was empty before write)
    ASSERT_TRUE(Sim_USART1.CR1 & USART_CR1_TXEIE);

    // Simulate TXE interrupt to verify data transmission
    Sim_USART1.SR |= USART_SR_TXE;
    USART1_IRQHandler();

    // Verify data was written to DR register
    ASSERT_EQ(Sim_USART1.DR, 'A');

    // One final interrupt to disable TXE (happens when last byte finishes transmitting)
    Sim_USART1.SR |= USART_SR_TXE;
    USART1_IRQHandler();

    // After buffer empties, TXE interrupt should be disabled
    ASSERT_FALSE(Sim_USART1.CR1 & USART_CR1_TXEIE);
}

TEST_F(UartDriverTest, Uart2WritesSingleByte)
{
    uint8_t data[] = {'A'};

    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_write(HAL_UART2, data, 1), HAL_STATUS_OK);

    // Verify TXE interrupt was enabled
    ASSERT_TRUE(Sim_USART2.CR1 & USART_CR1_TXEIE);

    // Simulate TXE interrupt
    Sim_USART2.SR |= USART_SR_TXE;
    USART2_IRQHandler();

    ASSERT_EQ(Sim_USART2.DR, 'A');

    // One final interrupt to disable TXE (happens when last byte finishes transmitting)
    Sim_USART2.SR |= USART_SR_TXE;
    USART2_IRQHandler();

    ASSERT_FALSE(Sim_USART2.CR1 & USART_CR1_TXEIE);
}

TEST_F(UartDriverTest, Uart1WritesMultipleBytes)
{
    const size_t DATA_LEN = 10;
    uint8_t data[DATA_LEN];

    // Create test pattern
    for (int i = 0; i < DATA_LEN; i++) {
        data[i] = 'A' + i;
    }

    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_write(HAL_UART1, data, DATA_LEN), HAL_STATUS_OK);

    // Verify TXE interrupt was enabled
    ASSERT_TRUE(Sim_USART1.CR1 & USART_CR1_TXEIE);

    // Simulate transmission of all bytes
    for (int i = 0; i < DATA_LEN; i++) {
        Sim_USART1.SR |= USART_SR_TXE;
        USART1_IRQHandler();
        ASSERT_EQ(Sim_USART1.DR, 'A' + i);
    }

    // One final interrupt to disable TXE (happens when last byte finishes transmitting)
    Sim_USART1.SR |= USART_SR_TXE;
    USART1_IRQHandler();

    // After all bytes transmitted, TXE interrupt should be disabled
    ASSERT_FALSE(Sim_USART1.CR1 & USART_CR1_TXEIE);
}

TEST_F(UartDriverTest, Uart2WritesMultipleBytes)
{
    const size_t DATA_LEN = 10;
    uint8_t data[DATA_LEN];

    for (int i = 0; i < DATA_LEN; i++) {
        data[i] = 'A' + i;
    }

    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_write(HAL_UART2, data, DATA_LEN), HAL_STATUS_OK);

    ASSERT_TRUE(Sim_USART2.CR1 & USART_CR1_TXEIE);

    for (int i = 0; i < DATA_LEN; i++) {
        Sim_USART2.SR |= USART_SR_TXE;
        USART2_IRQHandler();
        ASSERT_EQ(Sim_USART2.DR, 'A' + i);
    }

    // One final interrupt to disable TXE (happens when last byte finishes transmitting)
    Sim_USART2.SR |= USART_SR_TXE;
    USART2_IRQHandler();

    ASSERT_FALSE(Sim_USART2.CR1 & USART_CR1_TXEIE);
}

TEST_F(UartDriverTest, Uart1WritesMaxBufferSize)
{
    const size_t DATA_LEN = CIRCULAR_BUFFER_MAX_SIZE;
    uint8_t data[DATA_LEN];

    for (int i = 0; i < DATA_LEN; i++) {
        data[i] = (uint8_t)(i & 0xFF);
    }

    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_write(HAL_UART1, data, DATA_LEN), HAL_STATUS_OK);

    // Simulate transmission of all bytes
    for (int i = 0; i < DATA_LEN; i++) {
        Sim_USART1.SR |= USART_SR_TXE;
        USART1_IRQHandler();
        ASSERT_EQ(Sim_USART1.DR, (uint8_t)(i & 0xFF));
    }
}

TEST_F(UartDriverTest, Uart2WritesMaxBufferSize)
{
    const size_t DATA_LEN = CIRCULAR_BUFFER_MAX_SIZE;
    uint8_t data[DATA_LEN];

    for (int i = 0; i < DATA_LEN; i++) {
        data[i] = (uint8_t)(i & 0xFF);
    }

    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_write(HAL_UART2, data, DATA_LEN), HAL_STATUS_OK);

    for (int i = 0; i < DATA_LEN; i++) {
        Sim_USART2.SR |= USART_SR_TXE;
        USART2_IRQHandler();
        ASSERT_EQ(Sim_USART2.DR, (uint8_t)(i & 0xFF));
    }
}

TEST_F(UartDriverTest, Uart1WriteFailsWhenBufferFull)
{
    const size_t DATA_LEN = CIRCULAR_BUFFER_MAX_SIZE + 1; // One byte too many
    uint8_t data[DATA_LEN];

    for (int i = 0; i < DATA_LEN; i++) {
        data[i] = 'X';
    }

    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    // This should fail because we're trying to write more than buffer capacity
    ASSERT_EQ(hal_uart_write(HAL_UART1, data, DATA_LEN), HAL_STATUS_ERROR);

    // TXE interrupt should NOT be enabled since write failed
    ASSERT_FALSE(Sim_USART1.CR1 & USART_CR1_TXEIE);
}

TEST_F(UartDriverTest, Uart2WriteFailsWhenBufferFull)
{
    const size_t DATA_LEN = CIRCULAR_BUFFER_MAX_SIZE + 1;
    uint8_t data[DATA_LEN];

    for (int i = 0; i < DATA_LEN; i++) {
        data[i] = 'X';
    }

    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_write(HAL_UART2, data, DATA_LEN), HAL_STATUS_ERROR);
    ASSERT_FALSE(Sim_USART2.CR1 & USART_CR1_TXEIE);
}

TEST_F(UartDriverTest, Uart1WritePartialThenComplete)
{
    const size_t FIRST_WRITE = CIRCULAR_BUFFER_MAX_SIZE / 2;
    const size_t SECOND_WRITE = CIRCULAR_BUFFER_MAX_SIZE / 4;
    uint8_t data1[FIRST_WRITE];
    uint8_t data2[SECOND_WRITE];

    // Prepare test data
    for (int i = 0; i < FIRST_WRITE; i++) {
        data1[i] = 'A';
    }
    for (int i = 0; i < SECOND_WRITE; i++) {
        data2[i] = 'B';
    }

    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    // First write
    ASSERT_EQ(hal_uart_write(HAL_UART1, data1, FIRST_WRITE), HAL_STATUS_OK);

    // Partially transmit first write (transmit half)
    for (int i = 0; i < FIRST_WRITE / 2; i++) {
        Sim_USART1.SR |= USART_SR_TXE;
        USART1_IRQHandler();
        ASSERT_EQ(Sim_USART1.DR, 'A');
    }

    // Second write should succeed (there's still space)
    ASSERT_EQ(hal_uart_write(HAL_UART1, data2, SECOND_WRITE), HAL_STATUS_OK);

    // Complete transmission - should get remaining A's then B's
    for (int i = 0; i < (FIRST_WRITE / 2); i++) {
        Sim_USART1.SR |= USART_SR_TXE;
        USART1_IRQHandler();
        ASSERT_EQ(Sim_USART1.DR, 'A');
    }

    for (int i = 0; i < SECOND_WRITE; i++) {
        Sim_USART1.SR |= USART_SR_TXE;
        USART1_IRQHandler();
        ASSERT_EQ(Sim_USART1.DR, 'B');
    }

    // One final interrupt to disable TXE (happens when last byte finishes transmitting)
    Sim_USART1.SR |= USART_SR_TXE;
    USART1_IRQHandler();

    // TXE should be disabled after buffer empties
    ASSERT_FALSE(Sim_USART1.CR1 & USART_CR1_TXEIE);
}

TEST_F(UartDriverTest, Uart2WritePartialThenComplete)
{
    const size_t FIRST_WRITE = CIRCULAR_BUFFER_MAX_SIZE / 2;
    const size_t SECOND_WRITE = CIRCULAR_BUFFER_MAX_SIZE / 4;
    uint8_t data1[FIRST_WRITE];
    uint8_t data2[SECOND_WRITE];

    for (int i = 0; i < FIRST_WRITE; i++) {
        data1[i] = 'A';
    }
    for (int i = 0; i < SECOND_WRITE; i++) {
        data2[i] = 'B';
    }

    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);

    ASSERT_EQ(hal_uart_write(HAL_UART2, data1, FIRST_WRITE), HAL_STATUS_OK);

    for (int i = 0; i < FIRST_WRITE / 2; i++) {
        Sim_USART2.SR |= USART_SR_TXE;
        USART2_IRQHandler();
        ASSERT_EQ(Sim_USART2.DR, 'A');
    }

    ASSERT_EQ(hal_uart_write(HAL_UART2, data2, SECOND_WRITE), HAL_STATUS_OK);

    for (int i = 0; i < (FIRST_WRITE / 2); i++) {
        Sim_USART2.SR |= USART_SR_TXE;
        USART2_IRQHandler();
        ASSERT_EQ(Sim_USART2.DR, 'A');
    }

    for (int i = 0; i < SECOND_WRITE; i++) {
        Sim_USART2.SR |= USART_SR_TXE;
        USART2_IRQHandler();
        ASSERT_EQ(Sim_USART2.DR, 'B');
    }

    // One final interrupt to disable TXE (happens when last byte finishes transmitting)
    Sim_USART2.SR |= USART_SR_TXE;
    USART2_IRQHandler();

    ASSERT_FALSE(Sim_USART2.CR1 & USART_CR1_TXEIE);
}

TEST_F(UartDriverTest, WriteDoesNotEnableTXEWhenBufferNotEmpty)
{
    uint8_t data1[] = {'A'};
    uint8_t data2[] = {'B'};

    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    // First write - should enable TXE
    ASSERT_EQ(hal_uart_write(HAL_UART1, data1, 1), HAL_STATUS_OK);
    ASSERT_TRUE(Sim_USART1.CR1 & USART_CR1_TXEIE);

    // Second write while buffer not empty - should NOT re-enable TXE
    // (it's already enabled)
    Sim_USART1.CR1 &= ~USART_CR1_TXEIE; // Clear flag to test
    ASSERT_EQ(hal_uart_write(HAL_UART1, data2, 1), HAL_STATUS_OK);
    ASSERT_FALSE(Sim_USART1.CR1 & USART_CR1_TXEIE); // Should remain cleared
}

TEST_F(UartDriverTest, WriteHandlesRandomData)
{
    const size_t DATA_LEN = 50;
    uint8_t data_sent[DATA_LEN];

    // Generate random test data
    for (int i = 0; i < DATA_LEN; i++) {
        data_sent[i] = random_uint8();
    }

    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_write(HAL_UART1, data_sent, DATA_LEN), HAL_STATUS_OK);

    // Verify all random data transmits correctly
    for (int i = 0; i < DATA_LEN; i++) {
        Sim_USART1.SR |= USART_SR_TXE;
        USART1_IRQHandler();
        ASSERT_EQ(Sim_USART1.DR, data_sent[i]);
    }

    // One final interrupt to disable TXE (happens when last byte finishes transmitting)
    Sim_USART1.SR |= USART_SR_TXE;
    USART1_IRQHandler();

    ASSERT_FALSE(Sim_USART1.CR1 & USART_CR1_TXEIE);
}

TEST_F(UartDriverTest, WriteHandlesSpecialByteValues)
{
    uint8_t special_bytes[] = {0x00, 0xFF, 0x7F, 0x80, 0x55, 0xAA};
    size_t len = sizeof(special_bytes);

    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_write(HAL_UART2, special_bytes, len), HAL_STATUS_OK);

    for (int i = 0; i < len; i++) {
        Sim_USART2.SR |= USART_SR_TXE;
        USART2_IRQHandler();
        ASSERT_EQ(Sim_USART2.DR, special_bytes[i]);
    }
}

TEST_F(UartDriverTest, WriteWithExcessiveLengthRequest)
{
    const size_t HUGE_LEN = SIZE_MAX;
    uint8_t single_byte = 'X';

    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    // Should fail gracefully - can't fit SIZE_MAX bytes in buffer
    ASSERT_EQ(hal_uart_write(HAL_UART1, &single_byte, HUGE_LEN), HAL_STATUS_ERROR);

    // TXE interrupt should not be enabled
    ASSERT_FALSE(Sim_USART1.CR1 & USART_CR1_TXEIE);
}

// Interrupt timing edge cases
TEST_F(UartDriverTest, HandlesTXEInterruptWithEmptyBuffer)
{
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    // Simulate spurious TXE interrupt (no data to send)
    Sim_USART1.SR |= USART_SR_TXE;
    USART1_IRQHandler();

    // Should disable TXE interrupt
    ASSERT_FALSE(Sim_USART1.CR1 & USART_CR1_TXEIE);
}

// Configuration parameter handling
TEST_F(UartDriverTest, InitIgnoresConfigParameter)
{
    uint8_t data = 0;
    size_t data_len = 1;
    size_t bytes_read = 0;
    uint32_t timeout_ms = 0;

    uint32_t config_data = 0xDEADBEEF;
    ASSERT_EQ(hal_uart_init(HAL_UART1, &config_data), HAL_STATUS_OK);

    // Verify initialization proceeds normally regardless of config content
    // Can read a byte.
    Sim_USART1.DR = 'A';
    Sim_USART1.SR |= USART_SR_RXNE;
    USART1_IRQHandler();  // simulate interrupt

    ASSERT_EQ(hal_uart_read(HAL_UART1, &data, data_len, &bytes_read, timeout_ms), HAL_STATUS_OK);
    ASSERT_EQ(data, 'A');
}

TEST_F(UartDriverTest, ReadFailsOnUninitializedUart)
{
    uint8_t data[10];
    size_t bytes_read = 0;

    // Don't call hal_uart_init
    ASSERT_EQ(hal_uart_read(HAL_UART1, data, sizeof(data), &bytes_read, 0), HAL_STATUS_ERROR);
    ASSERT_EQ(hal_uart_read(HAL_UART2, data, sizeof(data), &bytes_read, 0), HAL_STATUS_ERROR);
}

TEST_F(UartDriverTest, WriteFailsOnUninitializedUart)
{
    uint8_t data[] = "test";

    // Don't call hal_uart_init
    ASSERT_EQ(hal_uart_write(HAL_UART1, data, sizeof(data)-1), HAL_STATUS_ERROR);
    ASSERT_EQ(hal_uart_write(HAL_UART2, data, sizeof(data)-1), HAL_STATUS_ERROR);
}

TEST_F(UartDriverTest, Uart1MultipleInitsFail)
{
    // First init should succeed
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    // Second init should fail
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_ERROR);

    // Operations should still work after failed re-init
    uint8_t data[] = "test";
    ASSERT_EQ(hal_uart_write(HAL_UART1, data, sizeof(data)-1), HAL_STATUS_OK);
}

TEST_F(UartDriverTest, Uart2MultipleInitsFail)
{
    // First init should succeed
    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);

    // Second init should fail
    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_ERROR);

    // Operations should still work after failed re-init
    uint8_t data[] = "test";
    ASSERT_EQ(hal_uart_write(HAL_UART2, data, sizeof(data)-1), HAL_STATUS_OK);
}

TEST_F(UartDriverTest, ReinitAfterDeinitSucceeds)
{
    // Init -> Deinit -> Init should work
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_deinit(HAL_UART1), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_deinit(HAL_UART2), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);
}

TEST_F(UartDriverTest, Uart1DeinitRestoresHardwareToSafeState)
{
    // Initialize UART1 and verify it's configured correctly
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    // Verify initial configuration is correct (lock in expected state)
    ASSERT_TRUE(Sim_RCC.APB2ENR & RCC_APB2ENR_USART1EN);     // Clock enabled
    ASSERT_TRUE(Sim_USART1.CR1 & USART_CR1_UE);              // UART enabled
    ASSERT_TRUE(Sim_USART1.CR1 & USART_CR1_TE);              // TX enabled
    ASSERT_TRUE(Sim_USART1.CR1 & USART_CR1_RE);              // RX enabled
    ASSERT_TRUE(Sim_USART1.CR1 & USART_CR1_RXNEIE);          // RX interrupt enabled
    ASSERT_FALSE(Sim_USART1.CR1 & USART_CR1_TXEIE);          // TX interrupt initially disabled
    ASSERT_TRUE(NVIC_IsIRQEnabled(USART1_IRQn));             // NVIC interrupt enabled

    // Add some data to buffers to verify they get cleared
    uint8_t write_data[] = "test_data";
    ASSERT_EQ(hal_uart_write(HAL_UART1, write_data, sizeof(write_data)-1), HAL_STATUS_OK);

    // Simulate some received data
    Sim_USART1.DR = 'A';
    Sim_USART1.SR |= USART_SR_RXNE;
    USART1_IRQHandler();

    // Call deinit
    ASSERT_EQ(hal_uart_deinit(HAL_UART1), HAL_STATUS_OK);

    // ========== Verify Hardware State After Deinit ==========

    // Critical interrupts should be disabled (safety requirement)
    ASSERT_FALSE(Sim_USART1.CR1 & USART_CR1_RXNEIE);         // RX interrupt disabled
    ASSERT_FALSE(Sim_USART1.CR1 & USART_CR1_TXEIE);          // TX interrupt disabled
    ASSERT_FALSE(NVIC_IsIRQEnabled(USART1_IRQn));            // NVIC interrupt disabled

    // UART should be disabled (prevents spurious activity)
    ASSERT_FALSE(Sim_USART1.CR1 & USART_CR1_UE);             // UART disabled

    // Note: Clock may remain enabled - this is implementation-dependent
    ASSERT_FALSE(Sim_RCC.APB2ENR & RCC_APB2ENR_USART1EN);
    // GPIO pins will remain configured

    // ========== Verify Software State After Deinit ==========

    // Operations should fail after deinit
    uint8_t read_data[10];
    size_t bytes_read = 0;
    uint8_t more_write_data[] = "fail";

    ASSERT_EQ(hal_uart_read(HAL_UART1, read_data, sizeof(read_data), &bytes_read, 0),
              HAL_STATUS_ERROR);
    ASSERT_EQ(hal_uart_write(HAL_UART1, more_write_data, sizeof(more_write_data)-1),
              HAL_STATUS_ERROR);

    // Multiple deints should be safe (idempotent)
    ASSERT_EQ(hal_uart_deinit(HAL_UART1), HAL_STATUS_ERROR); // Should fail gracefully
}

TEST_F(UartDriverTest, DeinitDoesNotAffectOtherUARTs)
{
    // Initialize both UARTs
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_init(HAL_UART2, nullptr), HAL_STATUS_OK);

    // Verify both are working
    uint8_t test_data[] = "test";
    ASSERT_EQ(hal_uart_write(HAL_UART1, test_data, sizeof(test_data)-1), HAL_STATUS_OK);
    ASSERT_EQ(hal_uart_write(HAL_UART2, test_data, sizeof(test_data)-1), HAL_STATUS_OK);

    // Deinit only UART1
    ASSERT_EQ(hal_uart_deinit(HAL_UART1), HAL_STATUS_OK);

    // UART1 should be disabled
    ASSERT_EQ(hal_uart_write(HAL_UART1, test_data, sizeof(test_data)-1), HAL_STATUS_ERROR);

    // UART2 should still work normally
    ASSERT_EQ(hal_uart_write(HAL_UART2, test_data, sizeof(test_data)-1), HAL_STATUS_OK);

    // UART2 hardware should be unaffected
    ASSERT_TRUE(Sim_USART2.CR1 & USART_CR1_UE);              // UART2 still enabled
    ASSERT_TRUE(Sim_USART2.CR1 & USART_CR1_RXNEIE);          // UART2 RX interrupt still enabled
    ASSERT_TRUE(NVIC_IsIRQEnabled(USART2_IRQn));             // UART2 NVIC still enabled
}

TEST_F(UartDriverTest, ReinitAfterDeinitRestoresFullFunctionality)
{
    // Initial setup and operation
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    uint8_t original_data[] = "original";
    ASSERT_EQ(hal_uart_write(HAL_UART1, original_data, sizeof(original_data)-1), HAL_STATUS_OK);

    // Deinit
    ASSERT_EQ(hal_uart_deinit(HAL_UART1), HAL_STATUS_OK);

    // Reinit should succeed
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    // Verify hardware is reconfigured correctly after reinit
    ASSERT_TRUE(Sim_RCC.APB2ENR & RCC_APB2ENR_USART1EN);     // Clock enabled
    ASSERT_TRUE(Sim_USART1.CR1 & USART_CR1_UE);              // UART enabled
    ASSERT_TRUE(Sim_USART1.CR1 & USART_CR1_TE);              // TX enabled
    ASSERT_TRUE(Sim_USART1.CR1 & USART_CR1_RE);              // RX enabled
    ASSERT_TRUE(Sim_USART1.CR1 & USART_CR1_RXNEIE);          // RX interrupt enabled
    ASSERT_TRUE(NVIC_IsIRQEnabled(USART1_IRQn));             // NVIC interrupt enabled

    // Verify functionality is restored
    uint8_t new_data[] = "reinit_test";
    ASSERT_EQ(hal_uart_write(HAL_UART1, new_data, sizeof(new_data)-1), HAL_STATUS_OK);

    // Verify read functionality
    Sim_USART1.DR = 'R';
    Sim_USART1.SR |= USART_SR_RXNE;
    USART1_IRQHandler();

    uint8_t read_data;
    size_t bytes_read = 0;
    ASSERT_EQ(hal_uart_read(HAL_UART1, &read_data, 1, &bytes_read, 0), HAL_STATUS_OK);
    ASSERT_EQ(bytes_read, 1);
    ASSERT_EQ(read_data, 'R');
}

TEST_F(UartDriverTest, DeinitClearsBufferState)
{
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    // Fill buffers with data
    uint8_t write_data[] = "buffer_data";
    ASSERT_EQ(hal_uart_write(HAL_UART1, write_data, sizeof(write_data)-1), HAL_STATUS_OK);

    // Add received data
    for (int i = 0; i < 10; i++) {
        Sim_USART1.DR = 'A' + i;
        Sim_USART1.SR |= USART_SR_RXNE;
        USART1_IRQHandler();
    }

    // Deinit should clear internal state
    ASSERT_EQ(hal_uart_deinit(HAL_UART1), HAL_STATUS_OK);

    // Reinit
    ASSERT_EQ(hal_uart_init(HAL_UART1, nullptr), HAL_STATUS_OK);

    // Buffers should be empty after reinit
    uint8_t read_data[20];
    size_t bytes_read = 0;
    ASSERT_EQ(hal_uart_read(HAL_UART1, read_data, sizeof(read_data), &bytes_read, 0), HAL_STATUS_OK);
    ASSERT_EQ(bytes_read, 0); // No data should remain from before deinit
}
