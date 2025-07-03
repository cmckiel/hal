#include "gtest/gtest.h"

extern "C" {
#include "stm32f4_hal.h"
#include "stm32f4_uart_util.h"
#include "stm32f4_uart1.h"
#include "registers.h"
#include "nvic.h"
}

// Allow test to directly call the ISR
extern "C" void USART1_IRQHandler(void);

class Uart1DriverTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear peripheral state before each test
        Sim_USART1 = {0};
        Sim_GPIOA = {0};
        Sim_RCC = {0};
    }
};

TEST_F(Uart1DriverTest, InitSetsRegistersCorrectly) {
    ASSERT_EQ(stm32f4_uart1_init(nullptr), HAL_STATUS_OK);
    EXPECT_TRUE(Sim_USART1.CR1 & USART_CR1_UE);
    EXPECT_TRUE(Sim_USART1.CR1 & USART_CR1_RE);
    EXPECT_TRUE(Sim_USART1.CR1 & USART_CR1_TE);
}

TEST_F(Uart1DriverTest, SimulateRxInterrupt) {
    stm32f4_uart1_init(nullptr);

    Sim_USART1.DR = 'A';
    Sim_USART1.SR |= USART_SR_RXNE;
    USART1_IRQHandler();  // simulate interrupt

    uint8_t buf[1];
    size_t bytes_read = 0;
    ASSERT_EQ(stm32f4_uart1_read(buf, 1, &bytes_read, 0), HAL_STATUS_OK);
    EXPECT_EQ(bytes_read, 1);
    EXPECT_EQ(buf[0], 'A');
}

TEST_F(Uart1DriverTest, SimulateTxInterrupt) {
    stm32f4_uart1_init(nullptr);

    uint8_t data = 'B';
    ASSERT_EQ(stm32f4_uart1_write(&data, 1), HAL_STATUS_OK);

    // Simulate TXE interrupt trigger
    Sim_USART1.SR |= USART_SR_TXE;
    USART1_IRQHandler();

    EXPECT_EQ(Sim_USART1.DR, 'B');
}

TEST_F(Uart1DriverTest, WriteEnablesTXEInterruptForEmptyBuffer)
{
    stm32f4_uart1_init(nullptr);

    // Assert the interrupt is not enabled.
    ASSERT_EQ(Sim_USART1.CR1 & USART_CR1_TXEIE, 0);

    // Write data.
    uint8_t data[4] = { 0, 1, 2, 3 };
    ASSERT_EQ(stm32f4_uart1_write(&data[0], sizeof(data)), HAL_STATUS_OK);

    // Assert the interrupt is enabled.
    ASSERT_EQ((Sim_USART1.CR1 & USART_CR1_TXEIE), USART_CR1_TXEIE);
}

TEST_F(Uart1DriverTest, ISRDisablesTXEInterruptForEmptyBuffer)
{
    /******* SETUP **********/
    stm32f4_uart1_init(nullptr);

    // Assert the interrupt is not enabled.
    ASSERT_EQ(Sim_USART1.CR1 & USART_CR1_TXEIE, 0);

    // Write data.
    uint8_t data[4] = { 0, 1, 2, 3 };
    ASSERT_EQ(stm32f4_uart1_write(&data[0], sizeof(data)), HAL_STATUS_OK);

    // Assert the interrupt is enabled.
    ASSERT_EQ((Sim_USART1.CR1 & USART_CR1_TXEIE), USART_CR1_TXEIE);

    /******* TEST **********/
    // Hardware becomes available for transmit.
    Sim_USART1.SR |= USART_SR_TXE;

    USART1_IRQHandler();
    ASSERT_EQ(Sim_USART1.DR, 0);

    USART1_IRQHandler();
    ASSERT_EQ(Sim_USART1.DR, 1);

    USART1_IRQHandler();
    ASSERT_EQ(Sim_USART1.DR, 2);

    USART1_IRQHandler();
    ASSERT_EQ(Sim_USART1.DR, 3);

    USART1_IRQHandler();
    // Assert the interrupt is not enabled.
    ASSERT_EQ(Sim_USART1.CR1 & USART_CR1_TXEIE, 0);
}
