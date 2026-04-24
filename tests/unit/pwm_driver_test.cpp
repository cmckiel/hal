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
// Timer init tests
/***********************************************************************/

TEST_F(PWMDriverTest, TimerInit_CalculatesPSC)
{
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(200));
    ASSERT_EQ(Sim_TIM1.PSC, 1);

    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(Sim_TIM1.PSC, 0);

    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(10));
    ASSERT_EQ(Sim_TIM1.PSC, 24);
}

TEST_F(PWMDriverTest, TimerInit_CalculatesARR)
{
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(200));
    ASSERT_EQ(Sim_TIM1.ARR, 39999);

    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(Sim_TIM1.ARR, 799);

    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(10));
    ASSERT_EQ(Sim_TIM1.ARR, 63999);
}

TEST_F(PWMDriverTest, TimerInit_StartsCounter)
{
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(200));

    // ARR preload enabled so software writes latch on update events.
    ASSERT_TRUE(Sim_TIM1.CR1 & TIM_CR1_ARPE);

    // MOE: Main output enable.
    ASSERT_TRUE(Sim_TIM1.BDTR & TIM_BDTR_MOE);

    // Counter is running.
    ASSERT_TRUE(Sim_TIM1.CR1 & TIM_CR1_CEN);
}

/***********************************************************************/
// Channel init tests — GPIO
/***********************************************************************/

TEST_F(PWMDriverTest, ChannelInit_CH1_ConfiguresGPIO)
{
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));

    // RCC clocks enabled by timer_init
    ASSERT_TRUE(Sim_RCC.AHB1ENR & RCC_AHB1ENR_GPIOAEN);
    ASSERT_TRUE(Sim_RCC.APB2ENR & RCC_APB2ENR_TIM1EN);

    // PA8 in alternate function mode (MODER bits [17:16] = 0b10)
    ASSERT_TRUE(Sim_GPIOA.MODER & (1 << 17));
    ASSERT_FALSE(Sim_GPIOA.MODER & (1 << 16));

    // Push-pull
    ASSERT_FALSE(Sim_GPIOA.OTYPER & (1 << 8));

    // No pull-up, no pull-down
    ASSERT_FALSE(Sim_GPIOA.PUPDR & (1 << 17));
    ASSERT_FALSE(Sim_GPIOA.PUPDR & (1 << 16));

    // AF1 (TIM1) — AFR[1] bits [3:0]
    ASSERT_EQ((Sim_GPIOA.AFR[1] & 0xFu), 1u);
}

TEST_F(PWMDriverTest, ChannelInit_CH2_ConfiguresGPIO)
{
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH2));

    // PA9 in alternate function mode (MODER bits [19:18] = 0b10)
    ASSERT_TRUE(Sim_GPIOA.MODER & (1 << 19));
    ASSERT_FALSE(Sim_GPIOA.MODER & (1 << 18));

    // Push-pull
    ASSERT_FALSE(Sim_GPIOA.OTYPER & (1 << 9));

    // No pull-up, no pull-down
    ASSERT_FALSE(Sim_GPIOA.PUPDR & (1 << 19));
    ASSERT_FALSE(Sim_GPIOA.PUPDR & (1 << 18));

    // AF1 (TIM1) — AFR[1] bits [7:4]
    ASSERT_EQ((Sim_GPIOA.AFR[1] >> 4) & 0xFu, 1u);
}

TEST_F(PWMDriverTest, ChannelInit_CH3_ConfiguresGPIO)
{
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH3));

    // PA10 in alternate function mode (MODER bits [21:20] = 0b10)
    ASSERT_TRUE(Sim_GPIOA.MODER & (1 << 21));
    ASSERT_FALSE(Sim_GPIOA.MODER & (1 << 20));

    // Push-pull
    ASSERT_FALSE(Sim_GPIOA.OTYPER & (1 << 10));

    // No pull-up, no pull-down
    ASSERT_FALSE(Sim_GPIOA.PUPDR & (1 << 21));
    ASSERT_FALSE(Sim_GPIOA.PUPDR & (1 << 20));

    // AF1 (TIM1) — AFR[1] bits [11:8]
    ASSERT_EQ((Sim_GPIOA.AFR[1] >> 8) & 0xFu, 1u);
}

TEST_F(PWMDriverTest, ChannelInit_CH4_ConfiguresGPIO)
{
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH4));

    // PA11 in alternate function mode (MODER bits [23:22] = 0b10)
    ASSERT_TRUE(Sim_GPIOA.MODER & (1 << 23));
    ASSERT_FALSE(Sim_GPIOA.MODER & (1 << 22));

    // Push-pull
    ASSERT_FALSE(Sim_GPIOA.OTYPER & (1 << 11));

    // No pull-up, no pull-down
    ASSERT_FALSE(Sim_GPIOA.PUPDR & (1 << 23));
    ASSERT_FALSE(Sim_GPIOA.PUPDR & (1 << 22));

    // AF1 (TIM1) — AFR[1] bits [15:12]
    ASSERT_EQ((Sim_GPIOA.AFR[1] >> 12) & 0xFu, 1u);
}

/***********************************************************************/
// Channel init tests — output compare registers
/***********************************************************************/

TEST_F(PWMDriverTest, ChannelInit_CH1_ConfiguresPeripheral)
{
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(200));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));

    // CCR1 preload enabled.
    ASSERT_TRUE(Sim_TIM1.CCMR1 & TIM_CCMR1_OC1PE);

    // Output starts forced low for safety.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);

    // Active high polarity.
    ASSERT_FALSE(Sim_TIM1.CCER & TIM_CCER_CC1P);
    ASSERT_FALSE(Sim_TIM1.CCER & TIM_CCER_CC1NP);

    // Channel 1 output enabled.
    ASSERT_TRUE(Sim_TIM1.CCER & TIM_CCER_CC1E);
}

TEST_F(PWMDriverTest, ChannelInit_CH2_ConfiguresPeripheral)
{
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(200));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH2));

    // CCR2 preload enabled.
    ASSERT_TRUE(Sim_TIM1.CCMR1 & TIM_CCMR1_OC2PE);

    // Output starts forced low for safety.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC2M) >> TIM_CCMR1_OC2M_Pos, 0b100u);

    // Active high polarity.
    ASSERT_FALSE(Sim_TIM1.CCER & TIM_CCER_CC2P);
    ASSERT_FALSE(Sim_TIM1.CCER & TIM_CCER_CC2NP);

    // Channel 2 output enabled.
    ASSERT_TRUE(Sim_TIM1.CCER & TIM_CCER_CC2E);
}

TEST_F(PWMDriverTest, ChannelInit_CH3_ConfiguresPeripheral)
{
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(200));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH3));

    // CCR3 preload enabled.
    ASSERT_TRUE(Sim_TIM1.CCMR2 & TIM_CCMR2_OC3PE);

    // Output starts forced low for safety.
    ASSERT_EQ((Sim_TIM1.CCMR2 & TIM_CCMR2_OC3M) >> TIM_CCMR2_OC3M_Pos, 0b100u);

    // Active high polarity.
    ASSERT_FALSE(Sim_TIM1.CCER & TIM_CCER_CC3P);
    ASSERT_FALSE(Sim_TIM1.CCER & TIM_CCER_CC3NP);

    // Channel 3 output enabled.
    ASSERT_TRUE(Sim_TIM1.CCER & TIM_CCER_CC3E);
}

TEST_F(PWMDriverTest, ChannelInit_CH4_ConfiguresPeripheral)
{
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(200));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH4));

    // CCR4 preload enabled.
    ASSERT_TRUE(Sim_TIM1.CCMR2 & TIM_CCMR2_OC4PE);

    // Output starts forced low for safety.
    ASSERT_EQ((Sim_TIM1.CCMR2 & TIM_CCMR2_OC4M) >> TIM_CCMR2_OC4M_Pos, 0b100u);

    // Active high polarity.
    ASSERT_FALSE(Sim_TIM1.CCER & TIM_CCER_CC4P);
    ASSERT_FALSE(Sim_TIM1.CCER & TIM_CCER_CC4NP);

    // Channel 4 output enabled.
    ASSERT_TRUE(Sim_TIM1.CCER & TIM_CCER_CC4E);
}

TEST_F(PWMDriverTest, ChannelInit_SecondChannelDoesNotClobberFirst)
{
    // Arrange: bring up CH1 at 50% duty cycle.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));
    hal_pwm_enable(HAL_PWM_CH1, true);
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 50);

    uint32_t ch1_ccr = Sim_TIM1.CCR1;
    uint32_t ch1_ocm = (Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos;
    ASSERT_NE(ch1_ccr, 0u);
    ASSERT_EQ(ch1_ocm, 0b110u); // PWM Mode 1

    // Act: initialize CH2.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH2));

    // Assert: CH1's CCR and OC mode are undisturbed.
    ASSERT_EQ(Sim_TIM1.CCR1, ch1_ccr);
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, ch1_ocm);
}

/***********************************************************************/
// Set duty cycle tests
/***********************************************************************/

TEST_F(PWMDriverTest, SetZeroDutyCycleResultsInForcedLow)
{
    // Arrange: Initialize driver, enable the output, and set to a non-zero duty-cycle.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));
    hal_pwm_enable(HAL_PWM_CH1, true);
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 25);

    // Confirm the driver has been "arranged" into the standard pwm mode.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b110u);

    // Act: Now, set duty cycle to zero.
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 0);

    // Assert: Forced low has been set.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);
}

TEST_F(PWMDriverTest, SetZeroDutyCycleResultsInForcedLowRegardlessOfEnable)
{
    // Arrange: Initialize driver and manually set mode from outside the driver (testing only)
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));
    Sim_TIM1.CCMR1 = (TIM1->CCMR1 & ~TIM_CCMR1_OC1M) | (0b110u << TIM_CCMR1_OC1M_Pos);
    // Confirm the driver has been "arranged" into the standard pwm mode.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b110u);

    // Act: Now, set duty cycle to zero.
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 0);

    // Assert: Forced low has been set.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);
}

TEST_F(PWMDriverTest, SetFullDutyCycleResultsInForcedHigh)
{
    // Arrange: Initialize driver, enable the output, and confirm default forced low mode.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));
    hal_pwm_enable(HAL_PWM_CH1, true);
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);

    // Act: Now, set duty cycle to 100%.
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 100);

    // Assert: Forced high has been set.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b101u);
}

TEST_F(PWMDriverTest, SetPartialDutyCycleResultsInPWMMode)
{
    // Arrange: Initialize driver, enable the output, and confirm default forced low mode.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));
    hal_pwm_enable(HAL_PWM_CH1, true);
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);

    // Act: Now, set duty cycle to 50%.
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 50);

    // Assert: PWM mode has been set.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b110u);
}

TEST_F(PWMDriverTest, SetsDutyCycleRegisterCorrectly)
{
    // Arrange: Initialize driver, enable the output, and confirm default forced low mode.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));
    hal_pwm_enable(HAL_PWM_CH1, true);
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);

    // Act: Now, set duty cycle to 30%.
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 30);

    // Assert: The CCR1 register shall be 30% of ARR register.
    uint16_t ccr = Sim_TIM1.CCR1;
    uint16_t arr = Sim_TIM1.ARR;
    double ccr_ratio = (double)ccr / (double)arr;

    ASSERT_TRUE(0.29 < ccr_ratio && ccr_ratio < 0.31);
}

TEST_F(PWMDriverTest, SetDutyCycleHandlesAboveMaxDutyCycle)
{
    // Arrange: Initialize driver and enable the output.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));
    hal_pwm_enable(HAL_PWM_CH1, true);

    // Act: Now, set duty cycle to 200%.
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 200);

    // Assert: Forced high has been set.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b101u);
}

TEST_F(PWMDriverTest, ChannelDutyCyclesAreIndependent)
{
    // Arrange: bring up two channels.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH2));
    hal_pwm_enable(HAL_PWM_CH1, true);
    hal_pwm_enable(HAL_PWM_CH2, true);

    // Act: set different duty cycles on each channel.
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 30);
    hal_pwm_set_duty_cycle(HAL_PWM_CH2, 70);

    // Assert: each CCR reflects its own duty cycle.
    double ch1_ratio = (double)Sim_TIM1.CCR1 / (double)Sim_TIM1.ARR;
    double ch2_ratio = (double)Sim_TIM1.CCR2 / (double)Sim_TIM1.ARR;

    ASSERT_TRUE(0.29 < ch1_ratio && ch1_ratio < 0.31);
    ASSERT_TRUE(0.69 < ch2_ratio && ch2_ratio < 0.71);
}

/***********************************************************************/
// Enable/disable tests
/***********************************************************************/

TEST_F(PWMDriverTest, DriverMustBeEnabled)
{
    // Arrange: Initialize driver and ensure forced low is the default.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);

    // Act: Set non-zero duty cycle.
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 40);

    // Assert: Without enable, forced low is still set and ccr1 is zero.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);
    ASSERT_EQ(Sim_TIM1.CCR1, 0);
}

TEST_F(PWMDriverTest, DriverMustBeEnabledPriorToSetDutyCycle)
{
    // Arrange: Initialize driver and ensure forced low is the default.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);

    // Act: Set non-zero duty cycle.
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 40);

    // Assert: Without enable, forced low is still set and ccr1 is zero.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);
    ASSERT_EQ(Sim_TIM1.CCR1, 0);

    // Act: Enable the driver.
    hal_pwm_enable(HAL_PWM_CH1, true);

    // Assert: Previous set_duty_cycle() call had no effect while driver was disabled,
    // forced low is still set and ccr1 is zero. Value was thrown away.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);
    ASSERT_EQ(Sim_TIM1.CCR1, 0);

    // Act: Set non-zero duty cycle again now that driver is enabled.
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 40);

    // Assert: With enable, PWM mode was set and ccr1 has a value.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b110u);
    ASSERT_NE(Sim_TIM1.CCR1, 0);
}

TEST_F(PWMDriverTest, DisablingDriverCutsOutput)
{
    // Arrange: Initialize driver, enable, and set pwm to a non-zero value.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));
    hal_pwm_enable(HAL_PWM_CH1, true);
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 65);
    // Confirm active pwm signal.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b110u);
    ASSERT_NE(Sim_TIM1.CCR1, 0);

    // Act: Disable the driver.
    hal_pwm_enable(HAL_PWM_CH1, false);

    // Assert: The driver is disabled.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);
}

TEST_F(PWMDriverTest, ReenablingDriverResumesPreviousDutyCycle)
{
    // Arrange: Initialize driver, enable, and set pwm to a non-zero value.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));
    hal_pwm_enable(HAL_PWM_CH1, true);
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 65);
    // Confirm active pwm signal.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b110u);
    ASSERT_NE(Sim_TIM1.CCR1, 0);
    // Store the current ccr and arr values.
    uint16_t arr = Sim_TIM1.ARR;
    uint16_t ccr = Sim_TIM1.CCR1;

    // Act: Disable the driver.
    hal_pwm_enable(HAL_PWM_CH1, false);

    // Assert: The driver is disabled.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);

    // Act: Re-enable the driver.
    hal_pwm_enable(HAL_PWM_CH1, true);

    // Assert: The driver is enabled and acting at the previous duty cycle.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b110u);
    ASSERT_EQ(Sim_TIM1.ARR, arr);
    ASSERT_EQ(Sim_TIM1.CCR1, ccr);
}

TEST_F(PWMDriverTest, ChannelEnableIsIndependent)
{
    // Arrange: bring up two channels at 50% each.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH2));
    hal_pwm_enable(HAL_PWM_CH1, true);
    hal_pwm_enable(HAL_PWM_CH2, true);
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 50);
    hal_pwm_set_duty_cycle(HAL_PWM_CH2, 50);

    // Confirm both channels are in PWM Mode 1.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b110u);
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC2M) >> TIM_CCMR1_OC2M_Pos, 0b110u);

    // Act: Disable only CH1.
    hal_pwm_enable(HAL_PWM_CH1, false);

    // Assert: CH1 is forced low, CH2 is still in PWM Mode 1.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u);
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC2M) >> TIM_CCMR1_OC2M_Pos, 0b110u);
}

/***********************************************************************/
// Set frequency tests
/***********************************************************************/

TEST_F(PWMDriverTest, SetFrequencySetsANewFrequency)
{
    // Arrange: Initialize the driver, enable, and confirm ARR & PSC values.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(200));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));
    hal_pwm_enable(HAL_PWM_CH1, true);
    ASSERT_EQ(Sim_TIM1.PSC, 1);
    ASSERT_EQ(Sim_TIM1.ARR, 39999);

    // Act: Set a new frequency.
    hal_pwm_set_frequency(20000);

    // Assert: New ARR and PSC values are correct for new input frequency.
    ASSERT_EQ(Sim_TIM1.PSC, 0);
    ASSERT_EQ(Sim_TIM1.ARR, 799);
}

TEST_F(PWMDriverTest, SetFrequencyRestoresPreviousDutyCycle)
{
    // Arrange: Initialize the driver, enable, and set a 30% duty cycle.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(200));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));
    hal_pwm_enable(HAL_PWM_CH1, true);
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 30);

    // Confirm the 30% duty cycle.
    uint16_t ccr = Sim_TIM1.CCR1;
    uint16_t arr = Sim_TIM1.ARR;
    double ccr_ratio = (double)ccr / (double)arr;
    ASSERT_TRUE(0.29 < ccr_ratio && ccr_ratio < 0.31);

    // Act: Set a new frequency.
    hal_pwm_set_frequency(20000);

    // Assert: The old ratio was maintained.
    ccr = Sim_TIM1.CCR1;
    arr = Sim_TIM1.ARR;
    ccr_ratio = (double)ccr / (double)arr;
    ASSERT_TRUE(0.29 < ccr_ratio && ccr_ratio < 0.31);
}

TEST_F(PWMDriverTest, SetFrequencyRescalesAllEnabledChannels)
{
    // Arrange: bring up two channels at different duty cycles.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(200));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH2));
    hal_pwm_enable(HAL_PWM_CH1, true);
    hal_pwm_enable(HAL_PWM_CH2, true);
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 30);
    hal_pwm_set_duty_cycle(HAL_PWM_CH2, 60);

    // Act: Change the base frequency.
    hal_pwm_set_frequency(20000);

    // Assert: Both channels preserved their duty-cycle ratios.
    double ch1_ratio = (double)Sim_TIM1.CCR1 / (double)Sim_TIM1.ARR;
    double ch2_ratio = (double)Sim_TIM1.CCR2 / (double)Sim_TIM1.ARR;

    ASSERT_TRUE(0.29 < ch1_ratio && ch1_ratio < 0.31);
    ASSERT_TRUE(0.59 < ch2_ratio && ch2_ratio < 0.61);
}

/***********************************************************************/
// Miscellaneous tests
/***********************************************************************/

TEST_F(PWMDriverTest, SetDutyCycleZeroDoesNotDependOnDriverEnable)
{
    // Arrange: Given an initialized and enabled driver operating at 50% duty cycle.
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_timer_init(20000));
    ASSERT_EQ(HAL_STATUS_OK, hal_pwm_channel_init(HAL_PWM_CH1));
    hal_pwm_enable(HAL_PWM_CH1, true);
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 50);

    // Confirm 50% duty cycle.
    uint16_t ccr = Sim_TIM1.CCR1;
    uint16_t arr = Sim_TIM1.ARR;
    double ccr_ratio = (double)ccr / (double)arr;
    ASSERT_TRUE(0.49 < ccr_ratio && ccr_ratio < 0.51);

    // Act: Disable driver, set 0% duty cycle, re-enable driver.
    hal_pwm_enable(HAL_PWM_CH1, false);
    hal_pwm_set_duty_cycle(HAL_PWM_CH1, 0);
    hal_pwm_enable(HAL_PWM_CH1, true);

    // Assert: Driver is forced low and CCR1 is zero.
    ASSERT_EQ((Sim_TIM1.CCMR1 & TIM_CCMR1_OC1M) >> TIM_CCMR1_OC1M_Pos, 0b100u); // 0b100 is code for forced low.
    ASSERT_EQ(Sim_TIM1.CCR1, 0);
}
