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
    ASSERT_EQ(Sim_TIM1.PSC, 12);

    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(20000));
    ASSERT_EQ(Sim_TIM1.PSC, 0);

    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(10));
    ASSERT_EQ(Sim_TIM1.PSC, 256);
}

TEST_F(PWMDriverTest, CalculatesARR)
{
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(200));
    ASSERT_EQ(Sim_TIM1.ARR, 64614);

    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(20000));
    ASSERT_EQ(Sim_TIM1.ARR, 8399);

    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(10));
    ASSERT_EQ(Sim_TIM1.ARR, 65368);
}
