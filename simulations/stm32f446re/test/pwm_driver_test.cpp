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

    }

    void TearDown() override {

    }
};

TEST_F(PWMDriverTest, SmokeTest)
{
    ASSERT_EQ(HAL_STATUS_ERROR, hal_pwm_init(20));
}
