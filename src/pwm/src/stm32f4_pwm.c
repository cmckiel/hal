/**
 * @file stm32f4_pwm.c
 * @brief STM32F4 implementation of PWM.
 *
 * Copyright (c) 2025 - 2026 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "pwm.h"
#include "stm32f4_hal.h"

#include <assert.h>

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
/// @brief PWM Mode 1: In upcounting, channel is active as long as CNT<CCR else inactive. Classic PWM.
#define OC_MODE_PWM_1       0b110u
/// @brief Forced low: Output pin forced low. (0% duty cycle)
#define OC_MODE_FORCED_LOW  0b100u
/// @brief Forced high: Output pin forced high (100% duty cycle)
#define OC_MODE_FORCED_HIGH 0b101u

// Per-channel enabled state. Indexed by hal_pwm_channel_t (CH1=0 ... CH4=3).
static bool pwm_channel_enabled[4] = { false, false, false, false };
static_assert(ARRAY_SIZE(pwm_channel_enabled) == _HAL_PWM_CH_MAX, "The size of channels enabled does not match number of channels");

/*********************************************************************************************/
// Forward declarations
/*********************************************************************************************/
static void configure_timer_clocks(void);
static void configure_channel_gpio(hal_pwm_channel_t channel);
static void configure_channel_preload(hal_pwm_channel_t channel);
static void configure_channel_ccer(hal_pwm_channel_t channel);
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
 * @brief Sets the output compare mode for the given channel.
 * @param ch  Target channel.
 * @param ocm Output compare mode - The important modes of operation are:
 *   1. @ref OC_MODE_PWM_1       Classic PWM signal. Valid duty cycles of 1% - 99%.
 *   2. @ref OC_MODE_FORCED_LOW  Output forced low. Duty cycle is 0%.
 *   3. @ref OC_MODE_FORCED_HIGH Output forced high. Duty cycle is 100%.
 */
static inline void tim1_ch_set_ocmode(hal_pwm_channel_t ch, uint32_t ocm)
{
    // CCMR: Capture/Compare Mode Register
    switch (ch)
    {
        case HAL_PWM_CH1:
            TIM1->CCMR1 = (TIM1->CCMR1 & ~TIM_CCMR1_OC1M) | (ocm << TIM_CCMR1_OC1M_Pos);
            break;
        case HAL_PWM_CH2:
            TIM1->CCMR1 = (TIM1->CCMR1 & ~TIM_CCMR1_OC2M) | (ocm << TIM_CCMR1_OC2M_Pos);
            break;
        case HAL_PWM_CH3:
            TIM1->CCMR2 = (TIM1->CCMR2 & ~TIM_CCMR2_OC3M) | (ocm << TIM_CCMR2_OC3M_Pos);
            break;
        case HAL_PWM_CH4:
            TIM1->CCMR2 = (TIM1->CCMR2 & ~TIM_CCMR2_OC4M) | (ocm << TIM_CCMR2_OC4M_Pos);
            break;
        default:
            break;
    }
}

/**
 * @brief Returns the current capture/compare register value for the given channel.
 */
static inline uint32_t tim1_ch_get_ccr(hal_pwm_channel_t ch)
{
    switch (ch)
    {
        case HAL_PWM_CH1: return TIM1->CCR1;
        case HAL_PWM_CH2: return TIM1->CCR2;
        case HAL_PWM_CH3: return TIM1->CCR3;
        case HAL_PWM_CH4: return TIM1->CCR4;
        default:          return 0;
    }
}

/**
 * @brief Writes the capture/compare register for the given channel.
 */
static inline void tim1_ch_set_ccr(hal_pwm_channel_t ch, uint32_t ccr)
{
    switch (ch)
    {
        case HAL_PWM_CH1: TIM1->CCR1 = (uint16_t)ccr; break;
        case HAL_PWM_CH2: TIM1->CCR2 = (uint16_t)ccr; break;
        case HAL_PWM_CH3: TIM1->CCR3 = (uint16_t)ccr; break;
        case HAL_PWM_CH4: TIM1->CCR4 = (uint16_t)ccr; break;
        default:          break;
    }
}

/**
 * @brief Returns true if the channel is currently in PWM Mode 1.
 */
static inline bool tim1_ch_is_pwm_mode1(hal_pwm_channel_t ch)
{
    switch (ch)
    {
        case HAL_PWM_CH1: return (TIM1->CCMR1 & TIM_CCMR1_OC1M) == (OC_MODE_PWM_1 << TIM_CCMR1_OC1M_Pos);
        case HAL_PWM_CH2: return (TIM1->CCMR1 & TIM_CCMR1_OC2M) == (OC_MODE_PWM_1 << TIM_CCMR1_OC2M_Pos);
        case HAL_PWM_CH3: return (TIM1->CCMR2 & TIM_CCMR2_OC3M) == (OC_MODE_PWM_1 << TIM_CCMR2_OC3M_Pos);
        case HAL_PWM_CH4: return (TIM1->CCMR2 & TIM_CCMR2_OC4M) == (OC_MODE_PWM_1 << TIM_CCMR2_OC4M_Pos);
        default:          return false;
    }
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
 * @brief Set a channel's output to 0% (hold output low).
 */
static inline void set_forced_inactive(hal_pwm_channel_t ch)
{
    tim1_ch_set_ocmode(ch, OC_MODE_FORCED_LOW);
    tim1_force_update();
}

/**
 * @brief Set a channel's output to 100% (hold output high).
 */
static inline void set_forced_active(hal_pwm_channel_t ch)
{
    tim1_ch_set_ocmode(ch, OC_MODE_FORCED_HIGH);
    tim1_force_update();
}

/**
 * @brief Set a channel to classic PWM Mode 1 (1%-99% duty cycle).
 */
static inline void set_pwm_mode1(hal_pwm_channel_t ch)
{
    tim1_ch_set_ocmode(ch, OC_MODE_PWM_1);
    tim1_force_update();
}

/*********************************************************************************************/
// Public Interface
/*********************************************************************************************/

hal_status_t hal_pwm_timer_init(uint32_t pwm_frequency_hz)
{
    uint16_t psc = 0;
    uint16_t arr = 1;

    configure_timer_clocks();
    compute_psc_arr(pwm_frequency_hz, &psc, &arr);

    TIM1->CR1 = 0;
    TIM1->PSC = psc;
    TIM1->ARR = arr;

    // Enable preload for ARR so software updates latch on update events.
    TIM1->CR1 |= TIM_CR1_ARPE;
    tim1_force_update();

    // MOE: Main output enable. Necessary to get output out of the timer and to the pins.
    TIM1->BDTR |= TIM_BDTR_MOE;

    // Start the counter.
    TIM1->CR1 |= TIM_CR1_CEN;

    return HAL_STATUS_OK;
}

hal_status_t hal_pwm_channel_init(hal_pwm_channel_t channel)
{
    if (!ENUM_IN_RANGE(channel, _HAL_PWM_CH_MIN, _HAL_PWM_CH_MAX))
    {
        return HAL_STATUS_ERROR;
    }

    configure_channel_gpio(channel);

    // Enable CCR preload so compare register updates latch on update events.
    configure_channel_preload(channel);

    // Start in a safe state. (0% PWM)
    set_forced_inactive(channel);

    // Set polarity to active high and enable the channel output.
    configure_channel_ccer(channel);

    pwm_channel_enabled[(int)channel] = false;

    return HAL_STATUS_OK;
}

hal_status_t hal_pwm_enable(hal_pwm_channel_t channel, bool enable)
{
    if (!ENUM_IN_RANGE(channel, _HAL_PWM_CH_MIN, _HAL_PWM_CH_MAX))
    {
        return HAL_STATUS_ERROR;
    }

    int idx = (int)channel;
    if (enable)
    {
        pwm_channel_enabled[idx] = true;
        // Resume the previous setting if there was one.
        if (tim1_ch_get_ccr(channel) != 0)
        {
            set_pwm_mode1(channel);
        }
    }
    else
    {
        pwm_channel_enabled[idx] = false;
        set_forced_inactive(channel);
    }

    return HAL_STATUS_OK;
}

hal_status_t hal_pwm_set_duty_cycle(hal_pwm_channel_t channel, uint8_t percent)
{
    if (!ENUM_IN_RANGE(channel, _HAL_PWM_CH_MIN, _HAL_PWM_CH_MAX))
    {
        return HAL_STATUS_ERROR;
    }

    // Always set low if called with 0%, regardless of pwm_channel_enabled, for safety.
    if (percent == 0)
    {
        set_forced_inactive(channel);
        tim1_ch_set_ccr(channel, 0);
        return HAL_STATUS_OK;
    }

    if (pwm_channel_enabled[(int)channel])
    {
        if (percent >= 100)
        {
            set_forced_active(channel);
            return HAL_STATUS_OK;
        }

        // percent is something between 1%-99%.
        set_pwm_mode1(channel);

        // CCR = round(percent/100 * (ARR+1))
        // Common rounding trick for integers:
        // result = (numerator * scale + divisor/2) / divisor;
        uint32_t arrp1 = (uint32_t)TIM1->ARR + 1u;
        uint32_t ccr   = ((uint32_t)percent * arrp1 + 50u) / 100u;

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

        tim1_ch_set_ccr(channel, ccr);
        // With OCxPE=1, CCR update latches on next UG/overflow. Force UG to apply now:
        tim1_force_update();
    }

    return HAL_STATUS_OK;
}

void hal_pwm_set_frequency(uint32_t pwm_frequency_hz)
{
    uint16_t psc = 0;
    uint16_t arr = 1;
    compute_psc_arr(pwm_frequency_hz, &psc, &arr);

    // Capture each enabled channel's duty-cycle ratio before ARR changes.
    float ratios[4]   = { 0.0f, 0.0f, 0.0f, 0.0f };
    bool  was_pwm_mode[4] = { false, false, false, false };

    for (int i = 0; i < 4; i++)
    {
        hal_pwm_channel_t ch = (hal_pwm_channel_t)i;
        if (pwm_channel_enabled[i] && tim1_ch_is_pwm_mode1(ch))
        {
            was_pwm_mode[i] = true;
            ratios[i]   = (float)tim1_ch_get_ccr(ch) / (float)(TIM1->ARR + 1u);
        }
    }

    apply_psc_arr(psc, arr);

    // Rescale CCR for any channel that was in PWM Mode 1 to preserve its duty-cycle ratio.
    for (int i = 0; i < 4; i++)
    {
        if (!pwm_channel_enabled[i] || !was_pwm_mode[i])
        {
            continue;
        }

        hal_pwm_channel_t ch    = (hal_pwm_channel_t)i;
        float             ratio = ratios[i];

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

            tim1_ch_set_ccr(ch, ccr);
            set_pwm_mode1(ch);
            tim1_force_update();
        }
        else if (ratio >= 1.0f)
        {
            set_forced_active(ch);
        }
        else
        { // ratio == 0.0f
            set_forced_inactive(ch);
        }
    }
}

/*********************************************************************************************/
// Private Functions
/*********************************************************************************************/

static void configure_timer_clocks(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
}

static void configure_channel_gpio(hal_pwm_channel_t channel)
{
    // TIM1 channels CH1-CH4 map to PA8-PA11 on alternate function AF1.
    switch (channel)
    {
        case HAL_PWM_CH1:
            // PA8: MODER bits [17:16] = 0b10 (alternate function)
            GPIOA->MODER |=  BIT_17;
            GPIOA->MODER &= ~BIT_16;
            GPIOA->OTYPER &= ~BIT_8;                          // push-pull
            GPIOA->PUPDR  &= ~(BIT_17 | BIT_16);             // no pull
            GPIOA->AFR[1] &= ~(0xFu << 0);
            GPIOA->AFR[1] |=  (1u   << 0);                   // AF1 (TIM1)
            break;

        case HAL_PWM_CH2:
            // PA9: MODER bits [19:18] = 0b10
            GPIOA->MODER |=  BIT_19;
            GPIOA->MODER &= ~BIT_18;
            GPIOA->OTYPER &= ~BIT_9;
            GPIOA->PUPDR  &= ~(BIT_19 | BIT_18);
            GPIOA->AFR[1] &= ~(0xFu << 4);
            GPIOA->AFR[1] |=  (1u   << 4);
            break;

        case HAL_PWM_CH3:
            // PA10: MODER bits [21:20] = 0b10
            GPIOA->MODER |=  BIT_21;
            GPIOA->MODER &= ~BIT_20;
            GPIOA->OTYPER &= ~BIT_10;
            GPIOA->PUPDR  &= ~(BIT_21 | BIT_20);
            GPIOA->AFR[1] &= ~(0xFu << 8);
            GPIOA->AFR[1] |=  (1u   << 8);
            break;

        case HAL_PWM_CH4:
            // PA11: MODER bits [23:22] = 0b10
            GPIOA->MODER |=  BIT_23;
            GPIOA->MODER &= ~BIT_22;
            GPIOA->OTYPER &= ~BIT_11;
            GPIOA->PUPDR  &= ~(BIT_23 | BIT_22);
            GPIOA->AFR[1] &= ~(0xFu << 12);
            GPIOA->AFR[1] |=  (1u   << 12);
            break;

        default:
            break;
    }

    // Optional: Set high speed?
}

static void configure_channel_preload(hal_pwm_channel_t channel)
{
    // Enable CCR preload (OCxPE) so compare register updates latch on update events.
    switch (channel)
    {
        case HAL_PWM_CH1: TIM1->CCMR1 |= TIM_CCMR1_OC1PE; break;
        case HAL_PWM_CH2: TIM1->CCMR1 |= TIM_CCMR1_OC2PE; break;
        case HAL_PWM_CH3: TIM1->CCMR2 |= TIM_CCMR2_OC3PE; break;
        case HAL_PWM_CH4: TIM1->CCMR2 |= TIM_CCMR2_OC4PE; break;
        default:          break;
    }
}

static void configure_channel_ccer(hal_pwm_channel_t channel)
{
    // CCER: Capture/Compare Enable Register
    // Set polarity to active high (CC1P=0, CC1NP=0) then enable the channel output.
    switch (channel)
    {
        case HAL_PWM_CH1:
            TIM1->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP);
            TIM1->CCER |=   TIM_CCER_CC1E;
            break;
        case HAL_PWM_CH2:
            TIM1->CCER &= ~(TIM_CCER_CC2P | TIM_CCER_CC2NP);
            TIM1->CCER |=   TIM_CCER_CC2E;
            break;
        case HAL_PWM_CH3:
            TIM1->CCER &= ~(TIM_CCER_CC3P | TIM_CCER_CC3NP);
            TIM1->CCER |=   TIM_CCER_CC3E;
            break;
        case HAL_PWM_CH4:
            TIM1->CCER &= ~(TIM_CCER_CC4P | TIM_CCER_CC4NP);
            TIM1->CCER |=   TIM_CCER_CC4E;
            break;
        default:
            break;
    }
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
