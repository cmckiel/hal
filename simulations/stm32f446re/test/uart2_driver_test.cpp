#include "gtest/gtest.h"

extern "C" {
#include "stm32f4_uart2.h"
#include "stm32f4_hal.h"
#include "registers.h"
#include "nvic.h"
}

// Allow test to directly call the ISR
extern "C" void USART2_IRQHandler(void);

class UartDriverTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear peripheral state before each test
        Sim_USART2 = {0};
        Sim_GPIOA = {0};
        Sim_RCC = {0};
    }
};

TEST_F(UartDriverTest, InitSetsRegistersCorrectly) {
    ASSERT_EQ(stm32f4_uart2_init(nullptr), HAL_STATUS_OK);
    EXPECT_TRUE(Sim_USART2.CR1 & USART_CR1_UE);
    EXPECT_TRUE(Sim_USART2.CR1 & USART_CR1_RE);
    EXPECT_TRUE(Sim_USART2.CR1 & USART_CR1_TE);
}

TEST_F(UartDriverTest, SimulateRxInterrupt) {
    stm32f4_uart2_init(nullptr);

    Sim_USART2.DR = 'A';
    Sim_USART2.SR |= USART_SR_RXNE;
    USART2_IRQHandler();  // simulate interrupt

    uint8_t buf[1];
    size_t bytes_read = 0;
    ASSERT_EQ(stm32f4_uart2_read(buf, 1, &bytes_read, 0), HAL_STATUS_OK);
    EXPECT_EQ(bytes_read, 1);
    EXPECT_EQ(buf[0], 'A');
}

TEST_F(UartDriverTest, SimulateTxInterrupt) {
    stm32f4_uart2_init(nullptr);

    uint8_t data = 'B';
    ASSERT_EQ(stm32f4_uart2_write(&data, 1), HAL_STATUS_OK);

    // Simulate TXE interrupt trigger
    Sim_USART2.SR |= USART_SR_TXE;
    USART2_IRQHandler();

    EXPECT_EQ(Sim_USART2.DR, 'B');
}
