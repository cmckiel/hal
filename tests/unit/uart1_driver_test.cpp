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

TEST_F(Uart1DriverTest, Uart1InitializesAllRegistersCorrectly)
{
    // Call UART1 init directly to test low-level register configuration
    ASSERT_EQ(stm32f4_uart1_init(), HAL_STATUS_OK);

    // ========== GPIO Configuration Verification ==========

    // Verify GPIOA clock is enabled
    ASSERT_TRUE(Sim_RCC.AHB1ENR & RCC_AHB1ENR_GPIOAEN);

    // Verify PA9 (UART1 TX) is configured as alternate function
    // MODER bits [19:18] should be 10 (alternate function mode)
    ASSERT_FALSE(Sim_GPIOA.MODER & BIT_18);  // Bit 18 should be 0
    ASSERT_TRUE(Sim_GPIOA.MODER & BIT_19);   // Bit 19 should be 1

    // Verify PA10 (UART1 RX) is configured as alternate function
    // MODER bits [21:20] should be 10 (alternate function mode)
    ASSERT_FALSE(Sim_GPIOA.MODER & BIT_20);  // Bit 20 should be 0
    ASSERT_TRUE(Sim_GPIOA.MODER & BIT_21);   // Bit 21 should be 1

    // Verify PA9 alternate function is set to AF07 (UART)
    // AFR[1] bits [7:4] should be 0111 (AF07)
    uint32_t pa9_af = (Sim_GPIOA.AFR[1] >> (PIN_1 * AF_SHIFT_WIDTH)) & 0xF;
    ASSERT_EQ(pa9_af, 0x7);  // AF07

    // Verify PA10 alternate function is set to AF07 (UART)
    // AFR[1] bits [11:8] should be 0111 (AF07)
    uint32_t pa10_af = (Sim_GPIOA.AFR[1] >> (PIN_2 * AF_SHIFT_WIDTH)) & 0xF;
    ASSERT_EQ(pa10_af, 0x7);  // AF07

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
