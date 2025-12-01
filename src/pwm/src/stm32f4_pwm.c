/**
 * @file stm32f4_pwm.c
 * @brief STM32F4 implementation of PWM.
 *
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "pwm.h"
#include "stm32f4_hal.h"

#ifdef DESKTOP_BUILD
#include "registers.h"
#include "nvic.h"
#else
#include "stm32f4xx.h"
#endif

// @todo Calculate this smartly starting from SYSCLK
#define TIM1_FREQ_HZ 16000000

/*********************************************************************************************/
// Common Output Compare Modes used to configure the pin's behavior when counting events occur.
/*********************************************************************************************/
/// @brief PWM Mode 1: In upcounting, channel 1 is active as long as TIM1_CNT<TIM1_CCR1 else inactive.
/// Classic PWM.
#define OC_MODE_PWM_1       0b110u
/// @brief Forced low: Output pin forced low. (0% duty cycle)
#define OC_MODE_FORCED_LOW  0b100u
/// @brief Forced high: Output pin forced high (100% duty cycle)
#define OC_MODE_FORCED_HIGH 0b101u

static bool pwm_state_enabled = false;

/*********************************************************************************************/
// Forward declarations
/*********************************************************************************************/
static void configure_gpios();
static void compute_psc_arr(uint32_t pwm_frequency_hz, uint16_t* psc_out, uint16_t* arr_out);

/*********************************************************************************************/
// Inline helpers
/*********************************************************************************************/
/**
 * @brief Force an update event: load preloaded ARR/CCR/PSC
 */
static inline void tim1_force_update(void)
{
    // EGR: Event Generation Register
    //  UG: Update Generate
    TIM1->EGR = TIM_EGR_UG;
}

/**
 * @brief Sets the output compare mode for channel one.
 * @param ocm Output compare mode - The important modes of
 * operation are as follows:
 *   1. @ref OC_MODE_PWM_1 Classic PWM signal. Valid duty cycles of 1% - 99%.
 *   2. @ref OC_MODE_FORCED_LOW Output forced low. Duty cycle is 0%.
 *   3. @ref OC_MODE_FORCED_HIGH Output forced high. Duty cycle is 100%.
 */
static inline void tim1_ch1_set_ocmode(uint32_t ocm)
{
    // CCMR: Capture/Compare Mode Register
    TIM1->CCMR1 = (TIM1->CCMR1 & ~TIM_CCMR1_OC1M) | (ocm << TIM_CCMR1_OC1M_Pos);
}

/**
 * @brief Apply prescaler (psc) and auto-reload (arr) values to their registers.
 * Determines the frequency of the PWM signal.
 */
static inline void apply_psc_arr(uint16_t psc, uint16_t arr)
{
    TIM1->CR1 &= ~TIM_CR1_CEN;      // stop counter during reprogram (optional, safer)
    TIM1->PSC  = psc;
    TIM1->ARR  = arr;
    tim1_force_update();
    TIM1->CR1 |= TIM_CR1_CEN;
}

/**
 * @brief Set PWM Duty Cycle to 0%. (Hold output low)
 */
static inline void set_forced_inactive(void)
{
    tim1_ch1_set_ocmode(OC_MODE_FORCED_LOW);
    tim1_force_update();
}

/**
 * @brief Set PWM Duty Cycle to 100%. (Hold output high)
 */
static inline void set_forced_active(void)
{
    tim1_ch1_set_ocmode(OC_MODE_FORCED_HIGH);
    tim1_force_update();
}

/**
 * @brief Set PWM Mode to classic PWM mode (1%-99% duty cycle)
 */
static inline void set_pwm_mode1(void)
{
    tim1_ch1_set_ocmode(OC_MODE_PWM_1);
    tim1_force_update();
}

/*********************************************************************************************/
// Public Interface
/*********************************************************************************************/

hal_status_t hal_pwm_init(uint32_t pwm_frequency_hz)
{
    // Safe default values for psc and arr.
    uint16_t psc = 0;
    uint16_t arr = 1;

    configure_gpios();
    compute_psc_arr(pwm_frequency_hz, &psc, &arr);

    TIM1->CR1 = 0;
    TIM1->PSC = psc;
    TIM1->ARR = arr;
    tim1_force_update();

    // Enable preload for ARR and CCR1
    TIM1->CCMR1 |= TIM_CCMR1_OC1PE;
    TIM1->CR1 |= TIM_CR1_ARPE;

    // Start in a safe state. (0% PWM)
    set_forced_inactive();

    // Set the polarity to be active high.
    TIM1->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP);

    // Enable output for channel 1
    TIM1->CCER |= TIM_CCER_CC1E;

    // MOE: Main output enable. Necessary to get the output out of the timer and
    // to the configured output pin.
    TIM1->BDTR |= TIM_BDTR_MOE;

    // Start the counter. Output is still forced low.
    TIM1->CR1 |= TIM_CR1_CEN;

    pwm_state_enabled = false;

    return HAL_STATUS_OK;
}

void hal_pwm_enable(bool enable)
{
    if (enable)
    {
        pwm_state_enabled = true;
        // Resume the previous setting if there was one.
        if (TIM1->CCR1 != 0)
        {
            set_pwm_mode1();
        }
    }
    else
    {
        pwm_state_enabled = false;
        set_forced_inactive();
    }
}

void hal_pwm_set_duty_cycle(uint8_t percent)
{
    // Always set pwm low if called with 0%, regardless of pwm_state_enabled,
    // for safety.
    if (percent == 0)
    {
        set_forced_inactive();
        TIM1->CCR1 = 0;
        return;
    }

    if (pwm_state_enabled)
    {
        if (percent >= 100)
        {
            set_forced_active();
            return;
        }

        // percent is something between 1%-99%.
        set_pwm_mode1();

        // Calulate CCR1
        uint32_t arrp1 = (uint32_t)TIM1->ARR + 1u;
        // CCR = round(percent/100 * (ARR+1))
        // Common rounding trick for integers:
        // result = (numerator * scale + divisor/2) / divisor;
        uint32_t ccr = ( (uint32_t)percent * arrp1 + 50u ) / 100u;

        // Avoid accidental 0%.
        if (ccr == 0u)
        {
            ccr = 1u;
        }

        // Keep within [1..ARR]
        if (ccr > TIM1->ARR)
        {
            ccr = TIM1->ARR;
        }

        // Apply to CCR1
        TIM1->CCR1 = (uint16_t)ccr;
        // With OC1PE=1, CCR update latches on next UG/overflow. Force UG to apply now:
        tim1_force_update();
    }
}

void hal_pwm_set_frequency(uint32_t pwm_frequency_hz)
{
    if (pwm_state_enabled)
    {
        // Compute new PSC/ARR and apply
        uint16_t psc=0;
        uint16_t arr=1;
        compute_psc_arr(pwm_frequency_hz, &psc, &arr);

        // Capture current ratio before changing ARR
        float ratio = 0.0f;
        if ((TIM1->CCMR1 & TIM_CCMR1_OC1M) == (OC_MODE_PWM_1 << TIM_CCMR1_OC1M_Pos))
        {
            ratio = (float)TIM1->CCR1 / (float)(TIM1->ARR + 1u);
        }

        apply_psc_arr(psc, arr);

        // Restore ratio if applicable
        if (ratio > 0.0f && ratio < 1.0f)
        {
            uint32_t arrp1 = (uint32_t)TIM1->ARR + 1u;
            uint32_t ccr   = (uint32_t)(ratio * (float)arrp1 + 0.5f);

            // Avoid accidental 0%.
            if (ccr == 0u)
            {
                ccr = 1u;
            }

            // Keep within [1..ARR]
            if (ccr > TIM1->ARR)
            {
                ccr = TIM1->ARR;
            }

            TIM1->CCR1 = (uint16_t)ccr;
            set_pwm_mode1();
            tim1_force_update();
        }
        else if (ratio >= 1.0f)
        {
            set_forced_active();
        }
        else
        { // ratio == 0.0f
            set_forced_inactive();
        }
    }
}

/*********************************************************************************************/
// Private Functions
/*********************************************************************************************/

static void configure_gpios()
{
    // Clocks
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;

    // Using PA8 as PWM pin. Set alternate function: 0b10.
    GPIOA->MODER |= (BIT_17);
    GPIOA->MODER &= ~(BIT_16);

    // Set push-pull
    GPIOA->OTYPER &= ~(BIT_8);

    // No pull-up, no pull-down: 0b00
    GPIOA->PUPDR &= ~(BIT_17);
    GPIOA->PUPDR &= ~(BIT_16);

    // Set to alternate function 1 (TIM1)
    GPIOA->AFR[1] &= ~(0xFu);
    GPIOA->AFR[1] |= 1;       // [3:0] = 0b0001 for AF1

    // Optional: Set high speed?
}

static void compute_psc_arr(uint32_t pwm_frequency_hz, uint16_t* psc_out, uint16_t* arr_out)
{
    // Ensure frequency is non-zero
    pwm_frequency_hz = pwm_frequency_hz ? pwm_frequency_hz : 1;
    // Ensure we clamp high side.
    pwm_frequency_hz = (pwm_frequency_hz > TIM1_FREQ_HZ) ? TIM1_FREQ_HZ : pwm_frequency_hz;

    // target_count: The number of ticks of the timer 1 clock we must count in
    // order to create a pwm of the requested hz.
    // Eg: 20 khz pwm request -> 16,000,000 / 20,000 = 800
    // This is used to generate a 20 khz clock by repeatedly counting from
    // 0 -> 799 -> 0 -> 799 -> 0 -> 799 -> ...
    // and counting each time we roll over.
    // 0        -> 1        -> 2        -> ...
    // The rollover count will be a 20 khz clock.
    uint32_t target_count = TIM1_FREQ_HZ / pwm_frequency_hz;

    // Determine the prescaler (psc): For slow pwm signals (say 200 Hz), we would need
    // to count very high. For a 16,000,000 timer 1 clock, this would mean repeatedly
    // counting to 16,000,000 / 200 = 80,000 ticks. The problem is that the counting
    // register (ARR) is only 16 bits. 80,000 = 0x13880 > 0xFFFF means we can't count
    // high enough using the 16 bit register. In order to solve this, the prescaler
    // allows us to divide the input clock when counting. For example, if we set psc to
    // 1, we get: 80,000 / (1 + 1) = 40,000 which is less than 0xFFFF = 65,535. Now we have
    // a number small enough that we can count to.
    //
    // The general formula is based on the following:
    //     target_count = (psc + 1)(arr + 1)
    // =>  target_count / (psc + 1) = (arr + 1)
    // but, arr <= 0xFFFF = 65,535 => (arr + 1) <= 65,536 = 0x10000
    // so, target_count / (psc + 1) <= 65536
    // =>  target_count / 65536 <= (psc + 1)
    //
    // So, at first, it seems like target_count / 0x10000 would do the trick if we
    // we remember to subtract 1 later. The problem is that int math rounds down.
    // Using our example earlier (200 Hz PWM):
    // target_count = 80,000 => 80,000 / 65,536 = 1.22 => (C rounds down) => 1.
    // then subtract 1 => 1 - 1 = 0
    // And psc of 0 implies (arr + 1) = 80,000 / (0 + 1) = 80,000
    // implies arr = 79,999 which is greater than 65,535 and cannot fit in arr. We
    // therefore would have a problem with floor(target_count / 0x10000).
    //
    // So, we need ceil(target_count / 0x10000) which can be calculated using an
    // integer math trick: ceil(a/b) = (a + b - 1) / b.
    // Which can be proved using the fact that any integer `a` can be represented by
    // a = q*b + r, where r is the remainder. If r is 0, there is no problem, and the
    // answer is q. If r > 0, then the answer will be ceil(a/b) = q+1.
    uint32_t psc = (target_count + 0xFFFF) / 0x10000;

    // We need to subtract 1 because under the hood the prescaler performs it's job by
    // counting, but starting at zero. So if above, we got psc = 2, we subtract 1 so we
    // get psc = 1. Then it counts: 0, 1, 0, 1, 0, 1, ...
    // And the rollover happens after 1, but there are 2 states of the counter.
    if (psc != 0)
    {
        psc -= 1;
    }

    // Ensure psc is in 16 bit bounds.
    if (psc > 0xFFFF)
    {
        psc = 0xFFFF;
    }

    // Now derive (arr + 1) from psc:
    uint32_t arr = (target_count / (psc + 1));

    // Ensure ARR is positive before subtracting to get the true ARR.
    if (arr > 0)
    {
        arr -= 1;
    }

    // Ensure ARR is never zero. Only perform the subtraction if it doesn't take us to zero.
    // ARR must never be zero because it ruins the counting loop 0 -> ARR -> 0 -> ARR -> ... and
    // floods the system with constant update events. A genuine PWM cannot be generated at all in
    // the case ARR = 0.
    if (arr == 0)
    {
        arr = 1;
    }

    // Ensure arr is in 16 bit bounds.
    if (arr > 0xFFFF)
    {
        arr = 0xFFFF;
    }

    if (psc_out && arr_out)
    {
        *psc_out = (uint16_t)psc;
        *arr_out = (uint16_t)arr;
    }
}
