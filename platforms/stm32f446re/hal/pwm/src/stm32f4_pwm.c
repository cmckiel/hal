#include "pwm.h"
#include "stm32f4_hal.h"

#ifdef SIMULATION_BUILD
#include "registers.h"
#include "nvic.h"
#else
#include "stm32f4xx.h"
#endif

// @todo Calculate this smartly starting from SYSCLK
#define TIM1_FREQ_HZ 168000000

static void configure_gpios();
static void configure_timer(uint32_t pwm_frequency_hz);

HalStatus_t hal_pwm_init(uint32_t pwm_frequency_hz)
{
    configure_gpios();
    configure_timer(pwm_frequency_hz);
    return HAL_STATUS_OK;
}

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

static void configure_timer(uint32_t pwm_frequency_hz)
{
    // Ensure frequency is non-zero
    pwm_frequency_hz = pwm_frequency_hz ? pwm_frequency_hz : 1;

    // target_count: The number of ticks of the timer 1 clock we must count in
    // order to create a pwm of the requested hz.
    // Eg: 20 khz pwm request -> 168,000,000 / 20,000 = 8400
    // This is used to generate a 20 khz clock by repeatedly counting from
    // 0 -> 8399 -> 0 -> 8399 -> 0 -> 8399 -> ...
    // and counting each time we roll over.
    // 0         -> 1         -> 2         -> ...
    // The rollover count will be a 20 khz clock.
    uint32_t target_count = TIM1_FREQ_HZ / pwm_frequency_hz;

    // Determine the prescaler (psc): For slow pwm signals (say 200 Hz), we would need
    // to count very high. For a 168,000,000 timer 1 clock, this would mean repeatedly
    // counting to 168,000,000 / 200 = 840,000 ticks. The problem is that the counting
    // register (ARR) is only 16 bits. 840,000 = 0xCD140 > 0xFFFF means we can't count
    // high enough using the 16 bit register. In order to solve this, the prescaler
    // allows us to divide the input clock when counting. For example, if we set psc to
    // 12, we get: 840,000 / (12 + 1) = 64,615 which is less than 0xFFFF = 65,535. Now we have
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
    // target_count = 840,000 => 840,000 / 65,536 = 12.8 => (C rounds down) => 12.
    // then subtract 1 => 12 - 1 = 11
    // And psc of 11 implies (arr + 1) = 840,000 / (11 + 1) = 70,000
    // implies arr = 69,999 which is greater than 65,535 and cannot fit in arr. We
    // therefore would have a problem with floor(target_count / 0x10000).
    //
    // So, we need ceil(target_count / 0x10000) which can be calculated using an
    // integer math trick: ceil(a/b) = (a + b - 1) / b.
    // Which can be proved using the fact that any integer `a` can be represented by
    // a = q*b + r, where r is the remainder. If r is 0, there is no problem, and the
    // answer is q. If r > 0, then the answer will be ceil(a/b) = q+1.
    uint32_t psc = (target_count + 0xFFFF) / 0x10000;

    // We need to subtract 1 because under the hood the prescaler performs it's job by
    // counting, but starting at zero. So if above, we got psc = 13, we subtract 1 so we
    // get psc = 12. Then it counts: 0, 1, 2, 3, ..., 10, 11, 12, 0, 1, 2, ...
    // And the rollover happens after 12, but there are 13 states of the counter.
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

    // Ensure (arr + 1) is positive before subtracting to get arr.
    if (arr != 0)
    {
        arr -= 1;
    }

    // Ensure arr is in 16 bit bounds.
    if (arr > 0xFFFF)
    {
        arr = 0xFFFF;
    }

    // Now we have the actual values to set PSC and ARR.
    TIM1->PSC = (uint16_t)psc;
    TIM1->ARR = (uint16_t)arr;
}
