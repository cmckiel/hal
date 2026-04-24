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
        Sim_GPIOB = {0};
        Sim_RCC = {0};
    }
};

TEST_F(Uart1DriverTest, Uart1InitializesAllRegistersCorrectly)
{
    // Call UART1 init directly to test low-level register configuration
    ASSERT_EQ(stm32f4_uart1_init(), HAL_STATUS_OK);

    // ========== GPIO Configuration Verification ==========

    // Verify GPIOB clock is enabled
    ASSERT_TRUE(Sim_RCC.AHB1ENR & RCC_AHB1ENR_GPIOBEN);

    // Verify PB6 (UART1 TX) is configured as alternate function
    // MODER bits [13:12] should be 10 (alternate function mode)
    ASSERT_FALSE(Sim_GPIOB.MODER & BIT_12);  // Bit 12 should be 0
    ASSERT_TRUE(Sim_GPIOB.MODER & BIT_13);   // Bit 13 should be 1

    // Verify PB7 (UART1 RX) is configured as alternate function
    // MODER bits [15:14] should be 10 (alternate function mode)
    ASSERT_FALSE(Sim_GPIOB.MODER & BIT_14);  // Bit 14 should be 0
    ASSERT_TRUE(Sim_GPIOB.MODER & BIT_15);   // Bit 15 should be 1

    // Verify PB6 alternate function is set to AF07 (UART)
    // AFR[0] bits [27:24] should be 0111 (AF07)
    uint32_t pb6_af = (Sim_GPIOB.AFR[0] >> (PIN_6 * AF_SHIFT_WIDTH)) & 0xF;
    ASSERT_EQ(pb6_af, 0x7);  // AF07

    // Verify PB7 alternate function is set to AF07 (UART)
    // AFR[0] bits [31:28] should be 0111 (AF07)
    uint32_t pb7_af = (Sim_GPIOB.AFR[0] >> (PIN_7 * AF_SHIFT_WIDTH)) & 0xF;
    ASSERT_EQ(pb7_af, 0x7);  // AF07

    // ========== UART Configuration Verification ==========

    // Verify USART1 clock is enabled
    ASSERT_TRUE(Sim_RCC.APB2ENR & RCC_APB2ENR_USART1EN);

    // Verify word length is 8 bits (M bit should be 0)
    ASSERT_FALSE(Sim_USART1.CR1 & USART_CR1_M);

    // Verify baud rate register is set correctly for 115200 baud
    // This should match the computed value from stm32f4_hal_compute_uart_bd()
    uint32_t expected_brr = stm32f4_hal_compute_uart_bd(APB2_CLK, 115200);
    ASSERT_EQ(Sim_USART1.BRR, expected_brr);

    // Verify transmitter is enabled
    ASSERT_TRUE(Sim_USART1.CR1 & USART_CR1_TE);

    // Verify receiver is enabled
    ASSERT_TRUE(Sim_USART1.CR1 & USART_CR1_RE);

    // Verify USART is enabled
    ASSERT_TRUE(Sim_USART1.CR1 & USART_CR1_UE);

    // Verify CR2 is set to default state (0)
    ASSERT_EQ(Sim_USART1.CR2, 0);

    // ========== Interrupt Configuration Verification ==========

    // Verify RXNE interrupt is enabled
    ASSERT_TRUE(Sim_USART1.CR1 & USART_CR1_RXNEIE);

    // Verify TXE interrupt is initially disabled
    ASSERT_FALSE(Sim_USART1.CR1 & USART_CR1_TXEIE);

    // Verify NVIC interrupt for USART1 is enabled
    ASSERT_TRUE(NVIC_IsIRQEnabled(USART1_IRQn));
}
