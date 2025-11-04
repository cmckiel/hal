#include "gtest/gtest.h"

extern "C" {
#include "pwm.h"
#include "registers.h"
#include "nvic.h"
#include "stm32f4_hal.h"
}

class PWMDriverTest : public ::testing::Test {
protected:
    void SetUp() override {
        Sim_RCC = { 0 };
        Sim_GPIOA = { 0 };
        Sim_TIM1 = { 0 };
    }

    void TearDown() override {

    }
};

TEST_F(PWMDriverTest, ConfiguresGPIOCorrectly)
{
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(20));

    // Clocks set up correctly
    ASSERT_TRUE(Sim_RCC.AHB1ENR & RCC_AHB1ENR_GPIOAEN);
    ASSERT_TRUE(Sim_RCC.APB2ENR & RCC_APB2ENR_TIM1EN);

    // PA8 in alternate function
    ASSERT_TRUE(Sim_GPIOA.MODER & (1 << 17));
    ASSERT_FALSE(Sim_GPIOA.MODER & (1 << 16));

    // Set push-pull
    ASSERT_FALSE(Sim_GPIOA.OTYPER & (1 << 8));

    // No pull-up, no pull-down
    ASSERT_FALSE(Sim_GPIOA.PUPDR & (1 << 17));
    ASSERT_FALSE(Sim_GPIOA.PUPDR & (1 << 16));

    // Set to alternate function 1 (TIM1)
    ASSERT_EQ((Sim_GPIOA.AFR[1] & 0xF), 1);
}

TEST_F(PWMDriverTest, CalculatesPSC)
{
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(200));
    ASSERT_EQ(Sim_TIM1.PSC, 1);

    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(20000));
    ASSERT_EQ(Sim_TIM1.PSC, 0);

    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(10));
    ASSERT_EQ(Sim_TIM1.PSC, 24);
}

TEST_F(PWMDriverTest, CalculatesARR)
{
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(200));
    ASSERT_EQ(Sim_TIM1.ARR, 39999);

    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(20000));
    ASSERT_EQ(Sim_TIM1.ARR, 799);

    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(10));
    ASSERT_EQ(Sim_TIM1.ARR, 63999);
}

TEST_F(PWMDriverTest, ConfiguresPeripheralCorrectly)
{
    // Upon calling init()...
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(200));

    // Ensure ARR (Auto Reload Register) and CCR1 (compare/compare register 1) are
    // preloaded. Meaning software updates to their values don't take place immediately but
    // instead happen simultaneously during timer update events.
    ASSERT_TRUE(Sim_TIM1.CCMR1 & TIM_CCMR1_OC1PE); // Output compare preload enable.
    ASSERT_TRUE(Sim_TIM1.CR1 & TIM_CR1_ARPE);      // Auto reload preload enable.

    // Ensure output starts forced low for safety reasons.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u); // 0b100 is code for forced low.

    // Ensure active high.
    ASSERT_FALSE(Sim_TIM1.CCER & TIM_CCER_CC1P);
    ASSERT_FALSE(Sim_TIM1.CCER & TIM_CCER_CC1NP);

    // Ensure output is enabled for channel 1.
    ASSERT_TRUE(Sim_TIM1.CCER & TIM_CCER_CC1E);

    // Ensure the main output is enabled.
    ASSERT_TRUE(Sim_TIM1.BDTR & TIM_BDTR_MOE);

    // Ensure the counter is started.
    ASSERT_TRUE(Sim_TIM1.CR1 & TIM_CR1_CEN);
}
