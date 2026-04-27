#include "gtest/gtest.h"
#include <climits>

extern "C" {
#include "systick.h"
#include "registers.h"

/* Exposed for overflow testing only — see systick.c DESKTOP_BUILD guard. */
extern uint32_t tick_ms;

extern void hal_systick_reset_for_test();

void SysTick_Handler(void);
}

#define CTRL_ENABLE  (1U << 0)
#define CTRL_TICKINT (1U << 1)
#define CTRL_CLKSRC  (1U << 2)

/* Simple counters used as callbacks. */
static int count_a;
static int count_b;
static int count_c;

static void cb_a(void) { count_a++; }
static void cb_b(void) { count_b++; }
static void cb_c(void) { count_c++; }

static void tick(int n)
{
    for (int i = 0; i < n; i++)
    {
        SysTick_Handler();
    }
}

class SysTickDriverTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        Sim_SysTick = {0};
        hal_systick_reset_for_test();
        count_a = count_b = count_c = 0;
    }
};

/***********************************************************************/
// hal_systick_init
/***********************************************************************/

TEST_F(SysTickDriverTest, Init_SetsLoadRegisterTo16000)
{
    hal_systick_init();
    ASSERT_EQ(Sim_SysTick.LOAD, 16000u);
}

TEST_F(SysTickDriverTest, Init_ClearsValueRegister)
{
    Sim_SysTick.VAL = 0xFFFF;
    hal_systick_init();
    ASSERT_EQ(Sim_SysTick.VAL, 0u);
}

TEST_F(SysTickDriverTest, Init_EnablesWithCorrectCtrlBits)
{
    hal_systick_init();
    ASSERT_TRUE(Sim_SysTick.CTRL & CTRL_ENABLE);
    ASSERT_TRUE(Sim_SysTick.CTRL & CTRL_CLKSRC);
    ASSERT_TRUE(Sim_SysTick.CTRL & CTRL_TICKINT);
}

TEST_F(SysTickDriverTest, Init_ReturnsOk)
{
    ASSERT_EQ(HAL_STATUS_OK, hal_systick_init());
}

/***********************************************************************/
// hal_get_tick
/***********************************************************************/

TEST_F(SysTickDriverTest, GetTick_ReturnsZeroAfterReset)
{
    ASSERT_EQ(0u, hal_get_tick());
}

TEST_F(SysTickDriverTest, GetTick_IncrementsByOnePerISRCall)
{
    tick(1);
    ASSERT_EQ(1u, hal_get_tick());

    tick(9);
    ASSERT_EQ(10u, hal_get_tick());

    tick(90);
    ASSERT_EQ(100u, hal_get_tick());
}

TEST_F(SysTickDriverTest, GetTick_WrapsAroundOnUint32Overflow)
{
    tick_ms = UINT32_MAX;
    tick(1);
    ASSERT_EQ(0u, hal_get_tick());
}

/***********************************************************************/
// hal_systick_timer_register — argument validation
/***********************************************************************/

TEST_F(SysTickDriverTest, Register_RejectsNullCallback)
{
    hal_timer_handle_t h = hal_systick_timer_register(100, HAL_TIMER_PERIODIC, NULL);
    ASSERT_EQ(HAL_TIMER_INVALID_HANDLE, h);
}

TEST_F(SysTickDriverTest, Register_RejectsZeroPeriod)
{
    hal_timer_handle_t h = hal_systick_timer_register(0, HAL_TIMER_PERIODIC, cb_a);
    ASSERT_EQ(HAL_TIMER_INVALID_HANDLE, h);
}

TEST_F(SysTickDriverTest, Register_ReturnsHandleInValidRange)
{
    hal_timer_handle_t h = hal_systick_timer_register(10, HAL_TIMER_PERIODIC, cb_a);
    ASSERT_GE(h, 0);
    ASSERT_LT(h, HAL_TIMER_MAX_CLIENTS);
}

TEST_F(SysTickDriverTest, Register_AssignsSequentialHandles)
{
    for (int i = 0; i < HAL_TIMER_MAX_CLIENTS; i++)
    {
        hal_timer_handle_t h = hal_systick_timer_register(10, HAL_TIMER_PERIODIC, cb_a);
        ASSERT_EQ(h, (hal_timer_handle_t)i);
    }
}

TEST_F(SysTickDriverTest, Register_ReturnsInvalidHandleWhenTableFull)
{
    for (int i = 0; i < HAL_TIMER_MAX_CLIENTS; i++)
    {
        hal_systick_timer_register(10, HAL_TIMER_PERIODIC, cb_a);
    }

    hal_timer_handle_t h = hal_systick_timer_register(10, HAL_TIMER_PERIODIC, cb_a);
    ASSERT_EQ(HAL_TIMER_INVALID_HANDLE, h);
}

/***********************************************************************/
// hal_systick_timer_deregister — argument validation
/***********************************************************************/

TEST_F(SysTickDriverTest, Deregister_NoOpForInvalidHandle)
{
    hal_systick_timer_register(5, HAL_TIMER_PERIODIC, cb_a);
    /* Deregistering with the sentinel value must not touch real slot 0. */
    hal_systick_timer_deregister(HAL_TIMER_INVALID_HANDLE);
    tick(5);
    ASSERT_EQ(1, count_a);
}

TEST_F(SysTickDriverTest, Deregister_NoOpForHandleEqualToMaxClients)
{
    hal_systick_timer_register(5, HAL_TIMER_PERIODIC, cb_a);
    hal_systick_timer_deregister((hal_timer_handle_t)HAL_TIMER_MAX_CLIENTS);
    tick(5);
    ASSERT_EQ(1, count_a);
}

TEST_F(SysTickDriverTest, Deregister_NoOpForInt8MinHandle)
{
    hal_systick_timer_register(5, HAL_TIMER_PERIODIC, cb_a);
    hal_systick_timer_deregister((hal_timer_handle_t)INT8_MIN);
    tick(5);
    ASSERT_EQ(1, count_a);
}

/***********************************************************************/
// Periodic timer — callback timing
/***********************************************************************/

TEST_F(SysTickDriverTest, Periodic_DoesNotFireBeforePeriodElapses)
{
    hal_systick_timer_register(10, HAL_TIMER_PERIODIC, cb_a);
    tick(9);
    ASSERT_EQ(0, count_a);
}

TEST_F(SysTickDriverTest, Periodic_FiresExactlyAtPeriod)
{
    hal_systick_timer_register(10, HAL_TIMER_PERIODIC, cb_a);
    tick(10);
    ASSERT_EQ(1, count_a);
}

TEST_F(SysTickDriverTest, Periodic_FiresRepeatedlyAtEveryPeriod)
{
    hal_systick_timer_register(5, HAL_TIMER_PERIODIC, cb_a);
    tick(15);
    ASSERT_EQ(3, count_a);
}

TEST_F(SysTickDriverTest, Periodic_PeriodOf1FiresEveryTick)
{
    hal_systick_timer_register(1, HAL_TIMER_PERIODIC, cb_a);
    tick(10);
    ASSERT_EQ(10, count_a);
}

TEST_F(SysTickDriverTest, Periodic_TwoTimersAtDifferentPeriodsFireIndependently)
{
    hal_systick_timer_register(3, HAL_TIMER_PERIODIC, cb_a);
    hal_systick_timer_register(5, HAL_TIMER_PERIODIC, cb_b);
    tick(15);
    ASSERT_EQ(5, count_a); /* 15 / 3 == 5 */
    ASSERT_EQ(3, count_b); /* 15 / 5 == 3 */
}

TEST_F(SysTickDriverTest, Periodic_StopsAfterDeregister)
{
    hal_timer_handle_t h = hal_systick_timer_register(5, HAL_TIMER_PERIODIC, cb_a);
    tick(5);
    ASSERT_EQ(1, count_a);

    hal_systick_timer_deregister(h);
    tick(5);
    ASSERT_EQ(1, count_a); /* count must not advance after deregister */
}

TEST_F(SysTickDriverTest, Periodic_DeregisterDoesNotAffectOtherTimers)
{
    hal_timer_handle_t h = hal_systick_timer_register(5, HAL_TIMER_PERIODIC, cb_a);
    hal_systick_timer_register(5, HAL_TIMER_PERIODIC, cb_b);

    tick(5);
    ASSERT_EQ(1, count_a);
    ASSERT_EQ(1, count_b);

    hal_systick_timer_deregister(h);
    tick(5);
    ASSERT_EQ(1, count_a);
    ASSERT_EQ(2, count_b);
}

/***********************************************************************/
// One-shot timer — callback timing
/***********************************************************************/

TEST_F(SysTickDriverTest, OneShot_DoesNotFireBeforePeriodElapses)
{
    hal_systick_timer_register(10, HAL_TIMER_ONE_SHOT, cb_a);
    tick(9);
    ASSERT_EQ(0, count_a);
}

TEST_F(SysTickDriverTest, OneShot_FiresExactlyAtPeriod)
{
    hal_systick_timer_register(10, HAL_TIMER_ONE_SHOT, cb_a);
    tick(10);
    ASSERT_EQ(1, count_a);
}

TEST_F(SysTickDriverTest, OneShot_FiresExactlyOnce)
{
    hal_systick_timer_register(5, HAL_TIMER_ONE_SHOT, cb_a);
    tick(100);
    ASSERT_EQ(1, count_a);
}

TEST_F(SysTickDriverTest, OneShot_FreesSlotAfterFiring)
{
    /* Fill the table with one-shot timers at period 1. */
    for (int i = 0; i < HAL_TIMER_MAX_CLIENTS; i++)
    {
        hal_systick_timer_register(1, HAL_TIMER_ONE_SHOT, cb_a);
    }

    /* One tick fires them all and frees every slot. */
    tick(1);
    ASSERT_EQ(HAL_TIMER_MAX_CLIENTS, count_a);

    /* Table should be empty — all 8 slots reusable. */
    for (int i = 0; i < HAL_TIMER_MAX_CLIENTS; i++)
    {
        hal_timer_handle_t h = hal_systick_timer_register(1, HAL_TIMER_ONE_SHOT, cb_b);
        ASSERT_GE(h, 0);
    }
}

/***********************************************************************/
// Table slot management
/***********************************************************************/

TEST_F(SysTickDriverTest, Register_ReusesSlotAfterDeregister)
{
    /* Fill table. */
    hal_timer_handle_t handles[HAL_TIMER_MAX_CLIENTS];
    for (int i = 0; i < HAL_TIMER_MAX_CLIENTS; i++)
    {
        handles[i] = hal_systick_timer_register(10, HAL_TIMER_PERIODIC, cb_a);
    }

    /* Table full — next registration fails. */
    ASSERT_EQ(HAL_TIMER_INVALID_HANDLE,
              hal_systick_timer_register(10, HAL_TIMER_PERIODIC, cb_a));

    /* Free slot 3. */
    hal_systick_timer_deregister(handles[3]);

    /* Slot 3 should now be reused. */
    hal_timer_handle_t h = hal_systick_timer_register(10, HAL_TIMER_PERIODIC, cb_b);
    ASSERT_EQ(3, h);
}

/***********************************************************************/
// Full table — all 8 timers fire correctly
/***********************************************************************/

TEST_F(SysTickDriverTest, AllEightPeriodicTimersFireCorrectly)
{
    for (int i = 0; i < HAL_TIMER_MAX_CLIENTS; i++)
    {
        hal_systick_timer_register(10, HAL_TIMER_PERIODIC, cb_a);
    }

    tick(30);
    ASSERT_EQ(HAL_TIMER_MAX_CLIENTS * 3, count_a);
}

/***********************************************************************/
// Tick overflow — timer callbacks survive uint32_t wrap
/***********************************************************************/

TEST_F(SysTickDriverTest, Periodic_SurvivesTickCounterOverflow)
{
    /* Place tick_ms so that the counter wraps mid-period. */
    tick_ms = UINT32_MAX - 2;

    hal_systick_timer_register(5, HAL_TIMER_PERIODIC, cb_a);
    tick(5);

    /* remaining_ms counts down independently of tick_ms; callback must fire. */
    ASSERT_EQ(1, count_a);
}

TEST_F(SysTickDriverTest, OneShot_SurvivesTickCounterOverflow)
{
    tick_ms = UINT32_MAX - 2;

    hal_systick_timer_register(5, HAL_TIMER_ONE_SHOT, cb_a);
    tick(5);

    ASSERT_EQ(1, count_a);

    /* Must not fire again. */
    tick(100);
    ASSERT_EQ(1, count_a);
}
