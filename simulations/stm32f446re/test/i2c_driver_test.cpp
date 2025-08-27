#include "gtest/gtest.h"

extern "C" {
#include "i2c.h"
#include "registers.h"
#include "nvic.h"
#include "stm32f4_hal.h"
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
        Sim_I2C1 = {0};

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

/***********************************************/
// Init Tests
/***********************************************/

TEST_F(I2CDriverTest, InitsGPIOPinsCorrectly)
{
    ASSERT_EQ(hal_i2c_init(nullptr), HAL_STATUS_OK);

    // Clock enabled to port b.
    ASSERT_TRUE(Sim_RCC.AHB1ENR & RCC_AHB1ENR_GPIOBEN);

    // pb8 in alt function mode
    ASSERT_FALSE(Sim_GPIOB.MODER & BIT_16); // Low
    ASSERT_TRUE(Sim_GPIOB.MODER & BIT_17);  // High

    // pb9 in alt function mode
    ASSERT_FALSE(Sim_GPIOB.MODER & BIT_18); // Low
    ASSERT_TRUE(Sim_GPIOB.MODER & BIT_19);  // High

    // pb8 alt func type to i2c
    uint32_t pb8_af = (Sim_GPIOB.AFR[1] >> (PIN_0 * AF_SHIFT_WIDTH)) & 0xF;
    ASSERT_EQ(pb8_af, AF4_MASK);

    // pb9 alt func type to i2c
    uint32_t pb9_af = (Sim_GPIOB.AFR[1] >> (PIN_1 * AF_SHIFT_WIDTH)) & 0xF;
    ASSERT_EQ(pb9_af, AF4_MASK);

    ASSERT_TRUE(Sim_GPIOB.OTYPER & (GPIO_OTYPER_OT_8 | GPIO_OTYPER_OT_9));
}

TEST_F(I2CDriverTest, InitsPeripheralCorrectly)
{
    // Setup
    // Set bit high to prove init() sets it low.
    Sim_I2C1.CCR |= I2C_CCR_FS;

    ASSERT_EQ(hal_i2c_init(nullptr), HAL_STATUS_OK);

    // Clock is enabled to I2C1
    ASSERT_TRUE(Sim_RCC.APB1ENR & RCC_APB1ENR_I2C1EN);

    // FREQ is set to 16 MHz
    ASSERT_EQ(Sim_I2C1.CR2 & I2C_CR2_FREQ, 16);

    // TRISE is set to 17
    ASSERT_EQ(Sim_I2C1.TRISE & I2C_TRISE_TRISE, 17);

    // CCR is set to 80 ticks.
    ASSERT_EQ(Sim_I2C1.CCR & I2C_CCR_CCR, 80);

    // Standard Speed Mode
    ASSERT_FALSE(Sim_I2C1.CCR & I2C_CCR_FS);

    // Peripheral is enabled
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_PE);
}

TEST_F(I2CDriverTest, InitsInterruptsCorrectly)
{
    ASSERT_EQ(hal_i2c_init(nullptr), HAL_STATUS_OK);

    // Event interrupt enabled in peripheral
    ASSERT_TRUE(Sim_I2C1.CR2 & I2C_CR2_ITEVTEN);

    // Error interrupt enabled in peripheral
    ASSERT_TRUE(Sim_I2C1.CR2 & I2C_CR2_ITERREN);

    // Event interrupt enabled in NVIC
    ASSERT_TRUE(NVIC_IsIRQEnabled(I2C1_EV_IRQn));

    // Error interrupt enabled in NVIC
    ASSERT_TRUE(NVIC_IsIRQEnabled(I2C1_ER_IRQn));
}

/***********************************************/
// Transaction Servicer Tests
/***********************************************/

TEST_F(I2CDriverTest, TransactionServicerLoadsTransaction)
{
    // Given an I2C transaction and an initialized I2C driver.
    HalI2C_Txn_t transaction;
    transaction.i2c_op = HAL_I2C_OP_WRITE; // @todo what happens if this isn't initialized or is out of bounds?
    ASSERT_EQ(hal_i2c_init(nullptr), HAL_STATUS_OK);

    // Given that the transaction has been submitted successfully.
    ASSERT_EQ(hal_i2c_submit_transaction(&transaction), HAL_STATUS_OK);
    ASSERT_EQ(transaction.processing_state, HAL_I2C_TXN_STATE_QUEUED);

    // When the transaction servicer is called, Then it should load the new transaction.
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_OK);
    ASSERT_EQ(transaction.processing_state, HAL_I2C_TXN_STATE_PROCESSING);

    // When the transaction servicer is called a second time, Then it should be busy processing the current transaction.
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_BUSY);
}
