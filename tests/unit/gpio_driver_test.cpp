#include "gtest/gtest.h"

extern "C" {
#include "gpio.h"
#include "registers.h"
}

#define LED_PIN (1 << 5)
#define GPIOAEN (1 << 0)

class GPIODriverTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear peripheral state before each test
        Sim_RCC = { 0 };
        Sim_GPIOA = { 0 };
    }

    void TearDown() override {

    }
};

TEST_F(GPIODriverTest, InitEnablesGpioAClockAndSetsPin5Output)
{
    hal_status_t status = hal_gpio_init();
    EXPECT_EQ(HAL_STATUS_OK, status);

    // GPIOA clock enabled?
    EXPECT_NE(Sim_RCC.AHB1ENR & GPIOAEN, 0u);

    // MODER bits for pin 5 (bits 11:10) should be 01 (output)
    uint32_t moder_bits = (Sim_GPIOA.MODER >> 10) & 0x3u;
    EXPECT_EQ(0x1u, moder_bits);
}

TEST_F(GPIODriverTest, ToggleLedFlipsPin5)
{
    // Ensure clean start
    Sim_GPIOA.ODR = 0;

    // First toggle -> sets LED pin
    hal_gpio_toggle_led();
    EXPECT_NE(Sim_GPIOA.ODR & LED_PIN, 0u);

    // Second toggle -> clears LED pin
    hal_gpio_toggle_led();
    EXPECT_EQ(Sim_GPIOA.ODR & LED_PIN, 0u);
}
