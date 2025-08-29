#include "gtest/gtest.h"

extern "C" {
#include "i2c.h"
#include "registers.h"
#include "nvic.h"
#include "stm32f4_hal.h"
#include "circular_buffer.h"
}

extern "C" {
    void I2C1_ER_IRQHandler(void);
    void I2C1_EV_IRQHandler(void);
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
    HalI2C_Txn_t transaction = {0};
    ASSERT_EQ(hal_i2c_init(nullptr), HAL_STATUS_OK);

    // Given that the transaction has been submitted successfully.
    ASSERT_EQ(hal_i2c_submit_transaction(&transaction), HAL_STATUS_OK);
    ASSERT_EQ(transaction.processing_state, HAL_I2C_TXN_STATE_QUEUED);

    // When the transaction servicer is called, then it should load the new transaction.
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_OK);
    ASSERT_EQ(transaction.processing_state, HAL_I2C_TXN_STATE_PROCESSING);

    // When the transaction servicer is called a second time, then it should be busy processing the current transaction.
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_BUSY);
}

TEST_F(I2CDriverTest, TransactionServicerRejectsInvalidTransaction)
{
    // Given an invalid I2C transaction and an initialized I2C driver.
    HalI2C_Txn_t transaction;
    transaction.i2c_op = _HAL_I2C_OP_MAX; // invalid operation value
    ASSERT_EQ(hal_i2c_init(nullptr), HAL_STATUS_OK);

    // @todo change how this the transaction is injected once this api also validates.
    // Given that the transaction has been submitted successfully.
    ASSERT_EQ(hal_i2c_submit_transaction(&transaction), HAL_STATUS_OK);
    ASSERT_EQ(transaction.processing_state, HAL_I2C_TXN_STATE_QUEUED);

    // When the transaction servicer is called, then it should reject the new transaction.
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_ERROR);
    ASSERT_EQ(transaction.processing_state, HAL_I2C_TXN_STATE_COMPLETED);
    ASSERT_EQ(transaction.transaction_result, HAL_I2C_TXN_RESULT_FAIL);

    // When the transaction servicer is called a second time, then it should be ready for the next transaction.
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_OK);
}

TEST_F(I2CDriverTest, TransactionServicerSendsStartSignal)
{
    // Given an I2C transaction and an initialized I2C driver.
    HalI2C_Txn_t transaction = {0};
    ASSERT_EQ(hal_i2c_init(nullptr), HAL_STATUS_OK);

    // Given that the transaction has been submitted successfully.
    ASSERT_EQ(hal_i2c_submit_transaction(&transaction), HAL_STATUS_OK);

    // When the transaction servicer is called,
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_OK);

    // Then the start signal shall be sent.
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_START);
}

TEST_F(I2CDriverTest, TransactionServicerSendsStartSignalOnlyOnce)
{
    // Given an I2C transaction and an initialized I2C driver.
    HalI2C_Txn_t transaction = {0};
    ASSERT_EQ(hal_i2c_init(nullptr), HAL_STATUS_OK);

    // Given that the transaction has been submitted successfully.
    ASSERT_EQ(hal_i2c_submit_transaction(&transaction), HAL_STATUS_OK);

    // Given that a transaction has been loaded and already sent the start signal.
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_OK);
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_START);

    // Given that the start signal is artificially reset.
    Sim_I2C1.CR1 &= ~I2C_CR1_START;
    ASSERT_FALSE(Sim_I2C1.CR1 & I2C_CR1_START);

    // When the transaction servicer is called a second time.
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_BUSY);

    // Then the start signal shall still be reset.
    ASSERT_FALSE(Sim_I2C1.CR1 & I2C_CR1_START);
}

/***********************************************/
// ISR Tests
/***********************************************/

TEST_F(I2CDriverTest, ISRHandlesBasicWrite)
{
    // Given an initialized I2C driver
    ASSERT_EQ(hal_i2c_init(nullptr), HAL_STATUS_OK);

    // Given a simple WRITE transaction of 2 bytes
    HalI2C_Txn_t txn = {};
    txn.target_addr               = 0x50;
    txn.i2c_op                    = HAL_I2C_OP_WRITE;
    txn.tx_data[0]                = 0x01;
    txn.tx_data[1]                = 0xAB;
    txn.num_of_bytes_to_tx        = 2;
    txn.expected_bytes_to_rx      = 0;
    txn.processing_state          = HAL_I2C_TXN_STATE_CREATED;
    txn.transaction_result        = HAL_I2C_TXN_RESULT_NONE;
    txn.actual_bytes_received     = 0;
    txn.actual_bytes_transmitted  = 0;
    memset(txn.rx_data, 0, sizeof(txn.rx_data));

    // Given the transaction was submitted successfully
    ASSERT_EQ(hal_i2c_submit_transaction(&txn), HAL_STATUS_OK);
    ASSERT_EQ(txn.processing_state, HAL_I2C_TXN_STATE_QUEUED);

    // When the servicer loads the transaction and issues START
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_OK);
    ASSERT_EQ(txn.processing_state, HAL_I2C_TXN_STATE_PROCESSING);
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_START);

    // Simulate ISR progress:

    // --------- SB phase ---------
    // HW-SIM: START condition sent -> SB set
    Sim_I2C1.SR1 |= I2C_SR1_SB;
    I2C1_EV_IRQHandler();
    // Assert address was written to data register in write mode.
    ASSERT_EQ(Sim_I2C1.DR, static_cast<uint32_t>((txn.target_addr << 1) | 0)); // write = 0

    // HW-SIM: User code reads SR1 -> HW clears SB then clears START shortly after
    Sim_I2C1.SR1 &= ~I2C_SR1_SB;
    Sim_I2C1.CR1 &= ~I2C_CR1_START;

    // --------- ADDR phase ---------
    // HW-SIM: target ACKs address -> HW sets ADDR
    Sim_I2C1.SR1 |= I2C_SR1_ADDR;
    I2C1_EV_IRQHandler();
    // Assert TXE and RXNE interrupts are enabled after successful address ack.
    ASSERT_TRUE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // HW-SIM: User code reads SR1 followed by SR2 -> HW clears ADDR
    Sim_I2C1.SR1 &= ~I2C_SR1_ADDR;

    // --------- TX phase ---------
    // Feed two TXE events. ISR should write both bytes, arming BTF after the last

    // --- TXE byte #1 ---
    // HW-SIM: Transmit buffer empty and ITBUFEN is enabled -> TXE is set
    Sim_I2C1.SR1 |= I2C_SR1_TXE;
    I2C1_EV_IRQHandler();
    // Assert the first data was written and STOP condition is not generated.
    ASSERT_EQ(Sim_I2C1.DR, static_cast<uint32_t>(0x01));
    ASSERT_FALSE(Sim_I2C1.CR1 & I2C_CR1_STOP);
    // HW-SIM: On real HW, TXE would clear on DR write then re-assert when shifter advances.
    // Here we just leave it to be re-set manually before the next ISR call.

    // --- TXE byte #2 ---
    // HW-SIM: Transmit buffer empty and ITBUFEN is enabled -> TXE is set
    Sim_I2C1.SR1 |= I2C_SR1_TXE;
    I2C1_EV_IRQHandler();
    // Assert the second data was written and STOP condition is not generated.
    ASSERT_EQ(Sim_I2C1.DR, static_cast<uint32_t>(0xAB));
    ASSERT_FALSE(Sim_I2C1.CR1 & I2C_CR1_STOP);

    // --------- BTF / STOP phase ---------
    // Finalize transmit with BTF; ISR should STOP and clear ITBUFEN for pure WRITE
    // HW-SIM: Last bit has been transmitted on the line -> BTF set.
    Sim_I2C1.SR1 |= I2C_SR1_BTF;
    I2C1_EV_IRQHandler();
    // Assert the STOP condition is set and that TXE and RXNE interrupts are disabled.
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_STOP);
    ASSERT_FALSE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // HW-SIM: HW clears BTF once a STOP condition is generated and STOP after.
    Sim_I2C1.SR1 &= ~I2C_SR1_BTF;
    Sim_I2C1.CR1 &= ~I2C_CR1_STOP;

    // --------- Assert results ---------
    // Then the next servicer call completes the transaction and copies results back
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_OK);
    ASSERT_EQ(txn.processing_state, HAL_I2C_TXN_STATE_COMPLETED);
    ASSERT_EQ(txn.transaction_result, HAL_I2C_TXN_RESULT_SUCCESS);
    ASSERT_EQ(txn.actual_bytes_transmitted, static_cast<size_t>(2));
    ASSERT_EQ(txn.actual_bytes_received, static_cast<size_t>(0));
}

TEST_F(I2CDriverTest, ISRHandlesAddressNACK)
{
    // Given an initialized I2C driver
    ASSERT_EQ(hal_i2c_init(nullptr), HAL_STATUS_OK);

    // Prepare a simple 1-byte WRITE so we can hit the address phase.
    HalI2C_Txn_t txn = {};
    txn.target_addr               = 0x2A;
    txn.i2c_op                    = HAL_I2C_OP_WRITE;
    txn.tx_data[0]                = 0xDE;
    txn.num_of_bytes_to_tx        = 1;
    txn.expected_bytes_to_rx      = 0;
    txn.processing_state          = HAL_I2C_TXN_STATE_CREATED;
    txn.transaction_result        = HAL_I2C_TXN_RESULT_NONE;
    txn.actual_bytes_received     = 0;
    txn.actual_bytes_transmitted  = 0;
    memset(txn.rx_data, 0, sizeof(txn.rx_data));

    // Given the transaction was submitted successfully
    ASSERT_EQ(hal_i2c_submit_transaction(&txn), HAL_STATUS_OK);
    ASSERT_EQ(txn.processing_state, HAL_I2C_TXN_STATE_QUEUED);

    // Given the servicer loads the transaction and asserts START
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_OK);
    ASSERT_EQ(txn.processing_state, HAL_I2C_TXN_STATE_PROCESSING);
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_START);

    // --------- SB phase ---------
    // HW-SIM: START condition sent -> SB set
    Sim_I2C1.SR1 |= I2C_SR1_SB;
    I2C1_EV_IRQHandler();
    // Assert address was written to data register in write mode.
    ASSERT_EQ(Sim_I2C1.DR, static_cast<uint32_t>((txn.target_addr << 1) | 0));

    // HW-SIM: HW would clear SB and then START shortly after
    Sim_I2C1.SR1 &= ~I2C_SR1_SB;
    Sim_I2C1.CR1 &= ~I2C_CR1_START;

    // --------- Address NACK error ---------
    // HW-SIM: target NACKs address -> AF set in SR1, error IRQ fires
    Sim_I2C1.SR1 |= I2C_SR1_AF;
    I2C1_ER_IRQHandler();

    // Error ISR should clear AF itself and abort:
    // - STOP requested
    // - ITBUFEN disabled
    // - transfer no longer in progress
    ASSERT_FALSE(Sim_I2C1.SR1 & I2C_SR1_AF);
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_STOP);
    ASSERT_FALSE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // Finalize: servicer should complete the transaction with FAIL
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_OK);
    ASSERT_EQ(txn.processing_state, HAL_I2C_TXN_STATE_COMPLETED);
    ASSERT_EQ(txn.transaction_result, HAL_I2C_TXN_RESULT_FAIL);
    ASSERT_EQ(txn.actual_bytes_transmitted, static_cast<size_t>(0));
    ASSERT_EQ(txn.actual_bytes_received,   static_cast<size_t>(0));
}
