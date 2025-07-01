#include "gtest/gtest.h"

extern "C" {
#include "uart.h"
#include "registers.h"
#include "nvic.h"
}

// Allow test to directly call the ISR
extern "C" void USART2_IRQHandler(void);

// Allow test access to UART internals.
extern "C" void _hal_uart_inject_rx_buffer_failure(HalUart_t);
extern "C" void _hal_uart_inject_tx_buffer_failure(HalUart_t);

class Uart2DriverTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear peripheral state before each test
        Sim_USART2 = {0};
        Sim_GPIOA = {0};
        Sim_RCC = {0};
    }
};

TEST_F(Uart2DriverTest, InitSuceeds)
{
    HalStatus_t status;
    status = hal_uart_init(HAL_UART2, nullptr);
    ASSERT_EQ(status, HAL_STATUS_OK);
}

TEST_F(Uart2DriverTest, InitFailsForRxBufferInitFailure)
{
    HalStatus_t status;

    // Arrange
    _hal_uart_inject_rx_buffer_failure(HAL_UART2);

    // Act
    status = hal_uart_init(HAL_UART2, nullptr);

    // Assert
    ASSERT_EQ(status, HAL_STATUS_ERROR);
}

TEST_F(Uart2DriverTest, InitFailsForTxBufferInitFailure)
{
    HalStatus_t status;

    // Arrange
    _hal_uart_inject_tx_buffer_failure(HAL_UART2);

    // Act
    status = hal_uart_init(HAL_UART2, nullptr);

    // Assert
    ASSERT_EQ(status, HAL_STATUS_ERROR);
}

TEST_F(Uart2DriverTest, EnablesGpioClock)
{
    HalStatus_t status;

    // Arrange
    ASSERT_EQ(Sim_RCC.AHB1ENR, 0);

    // Act
    status = hal_uart_init(HAL_UART2, nullptr);

    // Assert
    ASSERT_EQ(status, HAL_STATUS_OK);
    ASSERT_EQ(Sim_RCC.AHB1ENR & RCC_AHB1ENR_GPIOAEN, RCC_AHB1ENR_GPIOAEN);
}

// TEST_F(UartDriverTest, InitSetsRegistersCorrectly) {
//     ASSERT_EQ(stm32f4_uart2_init(nullptr), HAL_STATUS_OK);
//     EXPECT_TRUE(Sim_USART2.CR1 & USART_CR1_UE);
//     EXPECT_TRUE(Sim_USART2.CR1 & USART_CR1_RE);
//     EXPECT_TRUE(Sim_USART2.CR1 & USART_CR1_TE);
// }

// TEST_F(UartDriverTest, SimulateRxInterrupt) {
//     stm32f4_uart2_init(nullptr);

//     Sim_USART2.DR = 'A';
//     Sim_USART2.SR |= USART_SR_RXNE;
//     USART2_IRQHandler();  // simulate interrupt

//     uint8_t buf[1];
//     size_t bytes_read = 0;
//     ASSERT_EQ(stm32f4_uart2_read(buf, 1, &bytes_read, 0), HAL_STATUS_OK);
//     EXPECT_EQ(bytes_read, 1);
//     EXPECT_EQ(buf[0], 'A');
// }

// TEST_F(UartDriverTest, SimulateTxInterrupt) {
//     stm32f4_uart2_init(nullptr);

//     uint8_t data = 'B';
//     ASSERT_EQ(stm32f4_uart2_write(&data, 1), HAL_STATUS_OK);

//     // Simulate TXE interrupt trigger
//     Sim_USART2.SR |= USART_SR_TXE;
//     USART2_IRQHandler();

//     EXPECT_EQ(Sim_USART2.DR, 'B');
// }

// TEST_F(UartDriverTest, WriteEnablesTXEInterruptForEmptyBuffer)
// {
//     stm32f4_uart2_init(nullptr);

//     // Assert the interrupt is not enabled.
//     ASSERT_EQ(Sim_USART2.CR1 & USART_CR1_TXEIE, 0);

//     // Write data.
//     uint8_t data[4] = { 0, 1, 2, 3 };
//     ASSERT_EQ(stm32f4_uart2_write(&data[0], sizeof(data)), HAL_STATUS_OK);

//     // Assert the interrupt is enabled.
//     ASSERT_EQ((Sim_USART2.CR1 & USART_CR1_TXEIE), USART_CR1_TXEIE);
// }

// TEST_F(UartDriverTest, ISRDisablesTXEInterruptForEmptyBuffer)
// {
//     /******* SETUP **********/
//     stm32f4_uart2_init(nullptr);

//     // Assert the interrupt is not enabled.
//     ASSERT_EQ(Sim_USART2.CR1 & USART_CR1_TXEIE, 0);

//     // Write data.
//     uint8_t data[4] = { 0, 1, 2, 3 };
//     ASSERT_EQ(stm32f4_uart2_write(&data[0], sizeof(data)), HAL_STATUS_OK);

//     // Assert the interrupt is enabled.
//     ASSERT_EQ((Sim_USART2.CR1 & USART_CR1_TXEIE), USART_CR1_TXEIE);

//     /******* TEST **********/
//     // Hardware becomes available for transmit.
//     Sim_USART2.SR |= USART_SR_TXE;

//     USART2_IRQHandler();
//     ASSERT_EQ(Sim_USART2.DR, 0);

//     USART2_IRQHandler();
//     ASSERT_EQ(Sim_USART2.DR, 1);

//     USART2_IRQHandler();
//     ASSERT_EQ(Sim_USART2.DR, 2);

//     USART2_IRQHandler();
//     ASSERT_EQ(Sim_USART2.DR, 3);

//     USART2_IRQHandler();
//     // Assert the interrupt is not enabled.
//     ASSERT_EQ(Sim_USART2.CR1 & USART_CR1_TXEIE, 0);
// }
