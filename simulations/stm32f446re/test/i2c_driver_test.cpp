#include "gtest/gtest.h"

extern "C" {
#include "i2c.h"
#include "registers.h"
#include "nvic.h"
#include "circular_buffer.h"
}

// Helper for random test data.
// uint8_t random_uint8() {
//     return (uint8_t)(rand() % 256);
// }

class I2CDriverTest : public ::testing::Test {
private:
    // static bool seed_is_set;
    // int best_seed_ever = 42;
protected:
    void SetUp() override {
        // Clear peripheral state before each test
        // @todo Sim I2C
        Sim_GPIOB = {0};
        Sim_RCC = {0};

        // Set up the seed only once per full testing run.
        // if (!seed_is_set) {
        //     srand(best_seed_ever);
        //     seed_is_set = true;
        // }
    }

    void TearDown() override {
        hal_i2c_deinit();
    }
};

// bool I2CDriverTest::seed_is_set = false;

TEST_F(I2CDriverTest, InitEnablesGPIOBCorrectly)
{
    ASSERT_EQ(hal_i2c_init(nullptr), HAL_STATUS_OK);

    ASSERT_TRUE(Sim_RCC.AHB1ENR & RCC_AHB1ENR_GPIOBEN);
}
