#include "gtest/gtest.h"

extern "C" {
#include "stm32f4_hal.h"
#include "stm32f4_uart_util.h"
#include "stm32f4_uart2.h"
#include "registers.h"
#include "nvic.h"
}

// Allow test to directly call the ISR
extern "C" void USART2_IRQHandler(void);

class Uart2DriverTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear peripheral state before each test
        Sim_USART2 = {0};
        Sim_GPIOA = {0};
        Sim_RCC = {0};
    }
};

TEST_F(Uart2DriverTest, Uart2InitializesAllRegistersCorrectly)
{
    // Call UART2 init directly to test low-level register configuration
    ASSERT_EQ(stm32f4_uart2_init(nullptr), HAL_STATUS_OK);

    // ========== GPIO Configuration Verification ==========

    // Verify GPIOA clock is enabled
    ASSERT_TRUE(Sim_RCC.AHB1ENR & RCC_AHB1ENR_GPIOAEN);

    // Verify PA2 (UART2 TX) is configured as alternate function
    // MODER bits [5:4] should be 10 (alternate function mode)
    ASSERT_FALSE(Sim_GPIOA.MODER & BIT_4);   // Bit 4 should be 0
    ASSERT_TRUE(Sim_GPIOA.MODER & BIT_5);    // Bit 5 should be 1

    // Verify PA3 (UART2 RX) is configured as alternate function
    // MODER bits [7:6] should be 10 (alternate function mode)
    ASSERT_FALSE(Sim_GPIOA.MODER & BIT_6);   // Bit 6 should be 0
    ASSERT_TRUE(Sim_GPIOA.MODER & BIT_7);    // Bit 7 should be 1

    // Verify PA2 alternate function is set to AF07 (UART)
    // AFR[0] bits [11:8] should be 0111 (AF07)
    uint32_t pa2_af = (Sim_GPIOA.AFR[0] >> (PIN_2 * AF_SHIFT_WIDTH)) & 0xF;
    ASSERT_EQ(pa2_af, 0x7);  // AF07

    // Verify PA3 alternate function is set to AF07 (UART)
    // AFR[0] bits [15:12] should be 0111 (AF07)
    uint32_t pa3_af = (Sim_GPIOA.AFR[0] >> (PIN_3 * AF_SHIFT_WIDTH)) & 0xF;
    ASSERT_EQ(pa3_af, 0x7);  // AF07

    // ========== UART Configuration Verification ==========

    // Verify USART2 clock is enabled
    ASSERT_TRUE(Sim_RCC.APB1ENR & RCC_APB1ENR_USART2EN);

    // Verify word length is 8 bits (M bit should be 0)
    ASSERT_FALSE(Sim_USART2.CR1 & USART_CR1_M);

    // Verify baud rate register is set correctly for 115200 baud
    // This should match the computed value from stm32f4_hal_compute_uart_bd()
    uint32_t expected_brr = stm32f4_hal_compute_uart_bd(APB1_CLK, 115200);
    ASSERT_EQ(Sim_USART2.BRR, expected_brr);

    // Verify transmitter is enabled
    ASSERT_TRUE(Sim_USART2.CR1 & USART_CR1_TE);

    // Verify receiver is enabled
    ASSERT_TRUE(Sim_USART2.CR1 & USART_CR1_RE);

    // Verify USART is enabled
    ASSERT_TRUE(Sim_USART2.CR1 & USART_CR1_UE);

    // Verify CR2 is set to default state (0)
    ASSERT_EQ(Sim_USART2.CR2, 0);

    // ========== Interrupt Configuration Verification ==========

    // Verify RXNE interrupt is enabled
    ASSERT_TRUE(Sim_USART2.CR1 & USART_CR1_RXNEIE);

    // Verify TXE interrupt is initially disabled
    ASSERT_FALSE(Sim_USART2.CR1 & USART_CR1_TXEIE);

    // Verify NVIC interrupt for USART2 is enabled
    ASSERT_TRUE(NVIC_IsIRQEnabled(USART2_IRQn));
}
