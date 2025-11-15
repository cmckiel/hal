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

/***********************************************************************/
// Init tests
/***********************************************************************/

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

/***********************************************************************/
// Set duty cycle tests
/***********************************************************************/

TEST_F(PWMDriverTest, SetZeroDutyCycleResultsInForcedLow)
{
    // Arrange: Initialize driver, enable the output, and set to a non-zero duty-cycle.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(20000));
    hal_pwm_enable(true);
    hal_pwm_set_duty_cycle(25);

    // Confirm the driver has been "arranged" into the standard pwm mode.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b110u);

    // Act: Now, set duty cycle to zero.
    hal_pwm_set_duty_cycle(0);

    // Assert: Forced low has been set.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);
}

TEST_F(PWMDriverTest, SetZeroDutyCycleResultsInForcedLowRegardlessOfEnable)
{
    // Arrange: Initialize driver and manually set mode from outside the driver (testing only)
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(20000));
    Sim_TIM1.CCMR1 = (TIM1->CCMR1 & ~TIM_CCMR1_OC1M) | (0b110u << TIM_CCMR1_OC1M_Pos);
    // Confirm the driver has been "arranged" into the standard pwm mode.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b110u);

    // Act: Now, set duty cycle to zero.
    hal_pwm_set_duty_cycle(0);

    // Assert: Forced low has been set.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);
}

TEST_F(PWMDriverTest, SetFullDutyCycleResultsInForcedHigh)
{
    // Arrange: Initialize driver, enable the output, and confirm default forced low mode.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(20000));
    hal_pwm_enable(true);
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);

    // Act: Now, set duty cycle to 100%.
    hal_pwm_set_duty_cycle(100);

    // Assert: Forced high has been set.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b101u);
}

TEST_F(PWMDriverTest, SetPartialDutyCycleResultsInPWMMode)
{
    // Arrange: Initialize driver, enable the output, and confirm default forced low mode.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(20000));
    hal_pwm_enable(true);
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);

    // Act: Now, set duty cycle to 50%.
    hal_pwm_set_duty_cycle(50);

    // Assert: PWM mode has been set.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b110u);
}

TEST_F(PWMDriverTest, SetsDutyCycleRegisterCorrectly)
{
    // Arrange: Initialize driver, enable the output, and confirm default forced low mode.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(20000));
    hal_pwm_enable(true);
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);

    // Act: Now, set duty cycle to 30%.
    hal_pwm_set_duty_cycle(30);

    // Assert: The CCR1 register shall be 30% of ARR register.
    uint16_t ccr = Sim_TIM1.CCR1;
    uint16_t arr = Sim_TIM1.ARR;
    double ccr_ratio = (double)ccr / (double)arr;

    ASSERT_TRUE(0.29 < ccr_ratio && ccr_ratio < 0.31);
}

TEST_F(PWMDriverTest, SetDutyCycleHandlesAboveMaxDutyCycle)
{
    // Arrange: Initialize driver and enable the output.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(20000));
    hal_pwm_enable(true);

    // Act: Now, set duty cycle to 200%.
    hal_pwm_set_duty_cycle(200);

    // Assert: Forced high has been set.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b101u);
}

/***********************************************************************/
// Enable/disable tests
/***********************************************************************/

TEST_F(PWMDriverTest, DriverMustBeEnabled)
{
    // Arrange: Initialize driver and ensure forced low is the default.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(20000));
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);

    // Act: Set non-zero duty cycle.
    hal_pwm_set_duty_cycle(40);

    // Assert: Without enable, forced low is still set and ccr1 is zero.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);
    ASSERT_EQ(Sim_TIM1.CCR1, 0);
}

TEST_F(PWMDriverTest, DriverMustBeEnabledPriorToSetDutyCycle)
{
    // Arrange: Initialize driver and ensure forced low is the default.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(20000));
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);

    // Act: Set non-zero duty cycle.
    hal_pwm_set_duty_cycle(40);

    // Assert: Without enable, forced low is still set and ccr1 is zero.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);
    ASSERT_EQ(Sim_TIM1.CCR1, 0);

    // Act: Enable the driver.
    hal_pwm_enable(true);

    // Assert: Previous set_duty_cycle() call had no effect while driver was disabled,
    // forced low is still set and ccr1 is zero. Value was thrown away.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);
    ASSERT_EQ(Sim_TIM1.CCR1, 0);

    // Act: Set non-zero duty cycle again now that driver is enabled.
    hal_pwm_set_duty_cycle(40);

    // Assert: With enable, PWM mode was set and ccr1 has a value.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b110u);
    ASSERT_NE(Sim_TIM1.CCR1, 0);
}

TEST_F(PWMDriverTest, DisablingDriverCutsOutput)
{
    // Arrange: Initialize driver, enable, and set pwm to a non-zero value.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(20000));
    hal_pwm_enable(true);
    hal_pwm_set_duty_cycle(65);
    // Confirm active pwm signal.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b110u);
    ASSERT_NE(Sim_TIM1.CCR1, 0);

    // Act: Disable the driver.
    hal_pwm_enable(false);

    // Assert: The driver is disabled.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);
}

TEST_F(PWMDriverTest, ReenablingDriverResumesPreviousDutyCycle)
{
    // Arrange: Initialize driver, enable, and set pwm to a non-zero value.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_init(20000));
    hal_pwm_enable(true);
    hal_pwm_set_duty_cycle(65);
    // Confirm active pwm signal.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b110u);
    ASSERT_NE(Sim_TIM1.CCR1, 0);
    // Store the current ccr and arr values.
    uint16_t arr = Sim_TIM1.ARR;
    uint16_t ccr = Sim_TIM1.CCR1;

    // Act: Disable the driver.
    hal_pwm_enable(false);

    // Assert: The driver is disabled.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);

    // Act: Re-enable the driver.
    hal_pwm_enable(true);

    // Assert: The driver is enabled and acting at the previous duty cycle.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b110u);
    ASSERT_EQ(Sim_TIM1.ARR, arr);
    ASSERT_EQ(Sim_TIM1.CCR1, ccr);
}

/***********************************************************************/
// Set frequency tests
/***********************************************************************/
