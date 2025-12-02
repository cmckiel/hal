#include "gtest/gtest.h"

extern "C" {
#include "i2c.h"
#include "registers.h"
#include "nvic.h"
#include "stm32f4_hal.h"
#include "circular_buffer.h"

void I2C1_ER_IRQHandler(void);
void I2C1_EV_IRQHandler(void);
void _test_fixture_hal_i2c_reset_internals();
}

class I2CDriverTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Clear peripheral state before each test
        Sim_GPIOB = {0};
        Sim_RCC = {0};
        Sim_I2C1 = {0};

        _test_fixture_hal_i2c_reset_internals();
    }

    void TearDown() override {

    }
};

/***********************************************/
// Init Tests
/***********************************************/

TEST_F(I2CDriverTest, InitsGPIOPinsCorrectly)
{
    ASSERT_EQ(hal_i2c_init(), HAL_STATUS_OK);

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

    ASSERT_EQ(hal_i2c_init(), HAL_STATUS_OK);

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
    ASSERT_EQ(hal_i2c_init(), HAL_STATUS_OK);

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
    hal_i2c_txn_t transaction = {0};
    ASSERT_EQ(hal_i2c_init(), HAL_STATUS_OK);

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
    hal_i2c_txn_t transaction;
    transaction.i2c_op = _HAL_I2C_OP_MAX; // invalid operation value
    ASSERT_EQ(hal_i2c_init(), HAL_STATUS_OK);

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
    hal_i2c_txn_t transaction = {0};
    ASSERT_EQ(hal_i2c_init(), HAL_STATUS_OK);

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
    hal_i2c_txn_t transaction = {0};
    ASSERT_EQ(hal_i2c_init(), HAL_STATUS_OK);

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

TEST_F(I2CDriverTest, ISRHandlesZeroLengthTransmit)
{
    // Given an initialized I2C driver
    ASSERT_EQ(hal_i2c_init(), HAL_STATUS_OK);

    // Given a simple WRITE transaction of 0 bytes
    hal_i2c_txn_t txn = {};
    txn.target_addr               = 0x56;
    txn.i2c_op                    = HAL_I2C_OP_WRITE;
    txn.expected_bytes_to_tx      = 0;
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
    // There is no data to transmit.

    // HW-SIM: Transmit buffer empty and ITBUFEN is enabled -> TXE is set and interrupt generated.
    Sim_I2C1.SR1 |= I2C_SR1_TXE;
    I2C1_EV_IRQHandler();
    // The stop condition should be immediately generated and buffer interrupts disabled.
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_STOP);
    ASSERT_FALSE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // --------- Assert results ---------
    // Then the next servicer call completes the transaction and copies results back
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_OK);
    ASSERT_EQ(txn.processing_state, HAL_I2C_TXN_STATE_COMPLETED);
    ASSERT_EQ(txn.transaction_result, HAL_I2C_TXN_RESULT_SUCCESS);
    ASSERT_EQ(txn.actual_bytes_transmitted, static_cast<size_t>(0));
    ASSERT_EQ(txn.actual_bytes_received, static_cast<size_t>(0));
}

TEST_F(I2CDriverTest, ISRHandlesBasicWrite)
{
    // Given an initialized I2C driver
    ASSERT_EQ(hal_i2c_init(), HAL_STATUS_OK);

    // Given a simple WRITE transaction of 2 bytes
    hal_i2c_txn_t txn = {};
    txn.target_addr               = 0x50;
    txn.i2c_op                    = HAL_I2C_OP_WRITE;
    txn.tx_data[0]                = 0x01;
    txn.tx_data[1]                = 0xAB;
    txn.expected_bytes_to_tx      = 2;
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

TEST_F(I2CDriverTest, ISRHandlesBasicWriteRead)
{
    // Representing the I2C peripheral's internal shift register.
    uint32_t shift_register = 0;

    // Given an initialized I2C driver
    ASSERT_EQ(hal_i2c_init(), HAL_STATUS_OK);

    // Given a simple WRITE-READ transaction of 1 byte write, 2 byte read.
    hal_i2c_txn_t txn = {};
    txn.target_addr               = 0xFE;
    txn.i2c_op                    = HAL_I2C_OP_WRITE_READ;
    txn.expected_bytes_to_tx      = 1;
    txn.tx_data[0]                = 0xF4;
    txn.expected_bytes_to_rx      = 2;
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
    // Assert address was written to data register in read mode.
    ASSERT_EQ(Sim_I2C1.DR, static_cast<uint32_t>((txn.target_addr << 1) | 0)); // write = 0

    // HW-SIM: User code reads SR1 -> HW clears SB then clears START shortly after
    Sim_I2C1.SR1 &= ~I2C_SR1_SB;
    Sim_I2C1.CR1 &= ~I2C_CR1_START;

    // --------- ADDR phase ---------
    // HW-SIM: target ACKs address -> HW sets ADDR
    Sim_I2C1.SR1 |= I2C_SR1_ADDR;
    I2C1_EV_IRQHandler();
    // TxE interupts should be enabled for the transmit phase.
    ASSERT_TRUE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // HW-SIM: User code reads SR1 followed by SR2 -> HW clears ADDR
    Sim_I2C1.SR1 &= ~I2C_SR1_ADDR;

    // --------- TX phase ---------
    // Feed one TXE event. ISR should write the byte, arming BTF after the last

    // --- TXE byte #1 ---
    // HW-SIM: Transmit buffer empty and ITBUFEN is enabled -> TXE is set and interrupt generated.
    Sim_I2C1.SR1 |= I2C_SR1_TXE;
    I2C1_EV_IRQHandler();
    // Assert the first data was written and STOP condition is not generated.
    ASSERT_EQ(Sim_I2C1.DR, static_cast<uint32_t>(0xF4));
    ASSERT_FALSE(Sim_I2C1.CR1 & I2C_CR1_STOP);
    // HW-SIM: On real HW, TXE would clear on DR write then re-assert when shifter advances.
    // Here we just leave it to be re-set manually before the next ISR call.

    // HW-SIM: Data is fully sent onto the bus from the shift register and DR has not been written with
    // another byte -> BTF set and interrupt generated.
    Sim_I2C1.SR1 |= I2C_SR1_BTF;
    I2C1_EV_IRQHandler();
    // Assert that START condition was re-generated for the READ portion of transaction, and
    // that STOP is not set and TxE & RxNE interrupts disabled.
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_START);
    ASSERT_FALSE(Sim_I2C1.CR1 & I2C_CR1_STOP);
    ASSERT_FALSE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // HW-SIM: Clear BTF
    // @todo The behavior of the hardware needs to be better understood at this transition between write and read.
    Sim_I2C1.SR1 &= ~I2C_SR1_BTF;

    // --------- SB phase ---------
    // HW-SIM: START condition sent -> SB set
    Sim_I2C1.SR1 |= I2C_SR1_SB;
    I2C1_EV_IRQHandler();
    // Assert address was written to data register in read mode.
    ASSERT_EQ(Sim_I2C1.DR, static_cast<uint32_t>((txn.target_addr << 1) | 1)); // read = 1

    // HW-SIM: User code reads SR1 -> HW clears SB then clears START shortly after
    Sim_I2C1.SR1 &= ~I2C_SR1_SB;
    Sim_I2C1.CR1 &= ~I2C_CR1_START;

    // --------- ADDR phase ---------
    // HW-SIM: target ACKs address -> HW sets ADDR
    Sim_I2C1.SR1 |= I2C_SR1_ADDR;
    I2C1_EV_IRQHandler();
    // POS set and ACK reset for 2 byte rx phase
    // No RxNE interrupts.
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_POS);
    ASSERT_FALSE(Sim_I2C1.CR1 & I2C_CR1_ACK);
    ASSERT_FALSE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // HW-SIM: User code reads SR1 followed by SR2 -> HW clears ADDR
    Sim_I2C1.SR1 &= ~I2C_SR1_ADDR;

    // --------- RX phase ---------
    // We will deliver two data bytes: 0xA9, 0xB8

    // --- Byte #1 ---
    // HW-SIM: First byte received -> RXNE set but no iterrupt since
    // RxNE interrupts are not enabled.
    Sim_I2C1.DR  = static_cast<uint32_t>(0xA9);
    // Sim_I2C1.SR1 |= I2C_SR1_RXNE; // HW-SIM: Truthfully, the hardware sets this. But under test, I cannot
                                     // because the logic relies on hardware clearing this the moment DR is read, right in
                                     // the middle of the interrupt. I cannot simulate that in this single thread
                                     // of execution. RxNE therefore would stay set even after a DR read during this
                                     // test, which would invalidate the logic, and isn't how the hardware actually
                                     // works. See similar note just below this.

    // --- Byte #2 ---
    // HW-SIM: Next byte has arrived -> BTF and RXNE set,
    // DR contains 0xA9, Shift Register contains 0xB8.
    // Interrupt is generated by BTF.
    shift_register = static_cast<uint32_t>(0xB8);
    Sim_I2C1.SR1 |= I2C_SR1_BTF; // HW-SIM: And a little misleading. In reality, both BTF and RxNE are set right now.
                                 // But after the first read of DR in the BTF handler, the RxNE should be reset then and
                                 // there, in the middle of the interrupt. From this test suite I cannot simulate that.
                                 // Therefore, I leave RxNE reset to make sure I am testing the code paths I mean to.
    I2C1_EV_IRQHandler();

    // Driver should:
    //  - request STOP since all data has been received
    //  - reset POS
    //  - read DR -> store 0xA9
    //  - enable RxNE interrupts to catch last byte.
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_STOP);
    ASSERT_FALSE(Sim_I2C1.CR1 & I2C_CR1_POS);
    ASSERT_TRUE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // HW-SIM: Hardware clears BTF and RXNE after 0xA9 is read from DR and
    // moves 0xB8 from shift register to DR.
    Sim_I2C1.SR1 &= ~I2C_SR1_BTF;
    Sim_I2C1.SR1 &= ~I2C_SR1_RXNE;
    Sim_I2C1.CR1 &= ~I2C_CR1_STOP;
    Sim_I2C1.DR  = shift_register;

    // Moving shift register into DR sets RxNE and generates interrupt.
    Sim_I2C1.SR1 |= I2C_SR1_RXNE;
    I2C1_EV_IRQHandler();

    // Driver should disable RxNE interrupts.
    ASSERT_FALSE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // HW-SIM: Reading DR clears RxNE.
    Sim_I2C1.SR1 &= ~I2C_SR1_RXNE;

    // --------- Assert results ---------
    // Then the next servicer call completes the transaction and copies results back
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_OK);
    ASSERT_EQ(txn.processing_state, HAL_I2C_TXN_STATE_COMPLETED);
    ASSERT_EQ(txn.transaction_result, HAL_I2C_TXN_RESULT_SUCCESS);
    ASSERT_EQ(txn.actual_bytes_transmitted, static_cast<size_t>(1));
    ASSERT_EQ(txn.actual_bytes_received,   static_cast<size_t>(2));
    ASSERT_EQ(txn.rx_data[0], static_cast<uint8_t>(0xA9));
    ASSERT_EQ(txn.rx_data[1], static_cast<uint8_t>(0xB8));
}

TEST_F(I2CDriverTest, ISRHandlesBasicRead4Bytes)
{
    // Representing the I2C peripheral's internal shift register.
    uint32_t shift_register = 0;

    // Given an initialized I2C driver
    ASSERT_EQ(hal_i2c_init(), HAL_STATUS_OK);

    // Given a simple READ transaction of 4 bytes
    hal_i2c_txn_t txn = {};
    txn.target_addr               = 0xDD;
    txn.i2c_op                    = HAL_I2C_OP_READ;
    txn.expected_bytes_to_tx      = 0;
    txn.expected_bytes_to_rx      = 4;
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
    // Assert address was written to data register in read mode.
    ASSERT_EQ(Sim_I2C1.DR, static_cast<uint32_t>((txn.target_addr << 1) | 1)); // read = 1

    // HW-SIM: User code reads SR1 -> HW clears SB then clears START shortly after
    Sim_I2C1.SR1 &= ~I2C_SR1_SB;
    Sim_I2C1.CR1 &= ~I2C_CR1_START;

    // --------- ADDR phase ---------
    // HW-SIM: target ACKs address -> HW sets ADDR
    Sim_I2C1.SR1 |= I2C_SR1_ADDR;
    I2C1_EV_IRQHandler();
    // Assert ACK is set for multi-byte read and that RxNE and TxE interrupts are
    // disabled.
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_ACK);
    ASSERT_FALSE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // HW-SIM: User code reads SR1 followed by SR2 -> HW clears ADDR
    Sim_I2C1.SR1 &= ~I2C_SR1_ADDR;

    // --------- RX phase ---------
    // We will deliver four data bytes: 0xA9, 0xB8, 0xC7, 0xD6

    // --- Byte #1 ---
    // HW-SIM: First byte received -> RXNE set but no iterrupt since
    // RxNE interrupts are not enabled.
    Sim_I2C1.DR  = static_cast<uint32_t>(0xA9);
    Sim_I2C1.SR1 |= I2C_SR1_RXNE;

    // --- Byte #2 ---
    // HW-SIM: Next byte has arrived -> BTF set,
    // DR contains 0xA9, Shift Register contains 0xB8.
    // Interrupt is generated by BTF.
    shift_register = static_cast<uint32_t>(0xB8);
    Sim_I2C1.SR1 |= I2C_SR1_BTF;
    I2C1_EV_IRQHandler();

    // HW-SIM: Hardware clears BTF and RXNE after 0xA9 is read from DR and
    // moves 0xB8 from shift register to DR.
    Sim_I2C1.SR1 &= ~I2C_SR1_BTF;
    Sim_I2C1.SR1 &= ~I2C_SR1_RXNE;
    Sim_I2C1.DR  = shift_register;
    Sim_I2C1.SR1 |= I2C_SR1_RXNE; // moving data into DR from shift register sets RxNE.

    // --- Byte #3 ---
    // HW-SIM: Next byte has arrived -> BTF set,
    // DR contains 0xB8, Shift Register contains 0xC7.
    // Interrupt is generated by BTF.
    shift_register = static_cast<uint32_t>(0xC7);
    Sim_I2C1.SR1 |= I2C_SR1_BTF;
    I2C1_EV_IRQHandler();
    // Driver should clear ACK to NACK the last byte
    ASSERT_FALSE(Sim_I2C1.CR1 & I2C_CR1_ACK);

    // HW-SIM: Hardware clears BTF and RXNE after 0XB8 is read from DR and
    // moves 0xC7 from shift register to DR.
    Sim_I2C1.SR1 &= ~I2C_SR1_BTF;
    Sim_I2C1.SR1 &= ~I2C_SR1_RXNE;
    Sim_I2C1.DR  = shift_register;
    // Sim_I2C1.SR1 |= I2C_SR1_RXNE; // moving data into DR from shift register sets RxNE.
                                     // HW-SIM: Truthfully, the hardware sets this. But under test, I cannot
                                     // because the logic relies on hardware clearing this the moment DR is read, right in
                                     // the middle of the interrupt. I cannot simulate that in this single thread
                                     // of execution. RxNE therefore would stay set even after a DR read during this
                                     // test, which would invalidate the logic, and isn't how the hardware actually works.

    // --- Byte #4 arrives ---
    // HW-SIM: 0xD6 arrived in shift register.
    // DR contains 0xC7, shift register contains 0xD6.
    // Both RxNE and BTF set. Interrupt generated by BTF.
    shift_register = static_cast<uint32_t>(0xD6);
    Sim_I2C1.SR1 |= I2C_SR1_BTF;
    I2C1_EV_IRQHandler();
    // Assert STOP condition generated since all bytes have
    // been received.
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_STOP);
    // Assert RxNE interrupt is enabled so ISR can pick up the last
    // byte once it moves from shift register to DR.
    ASSERT_TRUE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // HW-SIM: HW clears BTF and RxNE after reading 0xC7 from DR and moves 0xD6 into DR.
    Sim_I2C1.SR1 &= ~I2C_SR1_BTF;
    Sim_I2C1.SR1 &= ~I2C_SR1_RXNE;
    Sim_I2C1.DR  = shift_register;
    // HW-SIM: HW clears STOP after it is generated.
    Sim_I2C1.CR1 &= ~I2C_CR1_STOP;

    // --- Byte #4 gets read ---
    // After 0xD6 gets moved from shift register to DR, RxNE interrupt is generated.
    Sim_I2C1.SR1 |= I2C_SR1_RXNE;
    I2C1_EV_IRQHandler();

    // Assert buffer IRQs should be disabled.
    ASSERT_FALSE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // --------- Assert results ---------
    // Then the next servicer call completes the transaction and copies results back
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_OK);
    ASSERT_EQ(txn.processing_state, HAL_I2C_TXN_STATE_COMPLETED);
    ASSERT_EQ(txn.transaction_result, HAL_I2C_TXN_RESULT_SUCCESS);
    ASSERT_EQ(txn.actual_bytes_transmitted, static_cast<size_t>(0));
    ASSERT_EQ(txn.actual_bytes_received,   static_cast<size_t>(4));
    ASSERT_EQ(txn.rx_data[0], static_cast<uint8_t>(0xA9));
    ASSERT_EQ(txn.rx_data[1], static_cast<uint8_t>(0xB8));
    ASSERT_EQ(txn.rx_data[2], static_cast<uint8_t>(0xC7));
    ASSERT_EQ(txn.rx_data[3], static_cast<uint8_t>(0xD6));
}

TEST_F(I2CDriverTest, ISRHandlesBasicRead3Bytes)
{
    // Representing the I2C peripheral's internal shift register.
    uint32_t shift_register = 0;

    // Given an initialized I2C driver
    ASSERT_EQ(hal_i2c_init(), HAL_STATUS_OK);

    // Given a simple READ transaction of 3 bytes
    hal_i2c_txn_t txn = {};
    txn.target_addr               = 0x3C;
    txn.i2c_op                    = HAL_I2C_OP_READ;
    txn.expected_bytes_to_tx      = 0;
    txn.expected_bytes_to_rx      = 3;
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
    // Assert address was written to data register in read mode.
    ASSERT_EQ(Sim_I2C1.DR, static_cast<uint32_t>((txn.target_addr << 1) | 1)); // read = 1

    // HW-SIM: User code reads SR1 -> HW clears SB then clears START shortly after
    Sim_I2C1.SR1 &= ~I2C_SR1_SB;
    Sim_I2C1.CR1 &= ~I2C_CR1_START;

    // --------- ADDR phase ---------
    // HW-SIM: target ACKs address -> HW sets ADDR
    Sim_I2C1.SR1 |= I2C_SR1_ADDR;
    I2C1_EV_IRQHandler();
    // Assert ACK is set for multi-byte read and that RxNE and TxE interrupts are
    // disabled.
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_ACK);
    ASSERT_FALSE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // HW-SIM: User code reads SR1 followed by SR2 -> HW clears ADDR
    Sim_I2C1.SR1 &= ~I2C_SR1_ADDR;

    // --------- RX phase ---------
    // We will deliver three data bytes: 0xA1, 0xB2, 0xC3

    // --- Byte #1 ---
    // HW-SIM: First byte received -> RXNE set but no iterrupt since
    // RxNE interrupts are not enabled.
    Sim_I2C1.DR  = static_cast<uint32_t>(0xA1);
    Sim_I2C1.SR1 |= I2C_SR1_RXNE;

    // --- Byte #2 ---
    // HW-SIM: Next byte has arrived -> BTF and RXNE set,
    // DR contains 0xA1, Shift Register contains 0xB2.
    // Interrupt is generated by BTF.
    shift_register = static_cast<uint32_t>(0xB2);
    Sim_I2C1.SR1 |= (I2C_SR1_BTF | I2C_SR1_RXNE);
    I2C1_EV_IRQHandler();
    // Driver should:
    //  - clear ACK to NACK the last byte
    //  - read DR -> store 0xA1
    ASSERT_FALSE(Sim_I2C1.CR1 & I2C_CR1_ACK);

    // HW-SIM: Hardware clears BTF and RXNE after 0XA1 is read from DR and
    // moves 0xB2 from shift register to DR.
    Sim_I2C1.SR1 &= ~I2C_SR1_BTF;
    Sim_I2C1.SR1 &= ~I2C_SR1_RXNE;
    Sim_I2C1.DR  = shift_register;

    // --- Byte #3 arrives ---
    // HW-SIM: 0xC3 arrived in shift register.
    // DR contains 0xB2, shift register contains 0xC3.
    // Both RxNE and BTF set. Interrupt generated by BTF.
    shift_register = static_cast<uint32_t>(0xC3);
    Sim_I2C1.SR1 |= I2C_SR1_BTF; // HW-SIM: And a little misleading. In reality, both BTF and RxNE are set right now.
                                 // But after the first read of DR in the BTF handler, the RxNE should be reset then and
                                 // there, in the middle of the interrupt. From this test suite I cannot simulate that.
                                 // Therefore, I leave RxNE reset to make sure I am testing the code paths I mean to.
    I2C1_EV_IRQHandler();
    // Assert STOP condition generated since all bytes have
    // been received.
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_STOP);
    // Assert RxNE interrupt is enabled so ISR can pick up the last
    // byte once it moves from shift register to DR.
    ASSERT_TRUE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // HW-SIM: HW clears BTF and RxNE after reading 0xB2 from DR and moves 0xC3 into DR.
    Sim_I2C1.SR1 &= ~I2C_SR1_BTF;
    Sim_I2C1.SR1 &= ~I2C_SR1_RXNE;
    Sim_I2C1.DR  = shift_register;
    // HW-SIM: HW clears STOP after it is generated.
    Sim_I2C1.CR1 &= ~I2C_CR1_STOP;

    // --- Byte #3 gets read ---
    // After 0xC3 gets moved from shift register to DR, RxNE interrupt is generated.
    Sim_I2C1.SR1 |= I2C_SR1_RXNE;
    I2C1_EV_IRQHandler();

    // Assert buffer IRQs should be disabled.
    ASSERT_FALSE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // --------- Assert results ---------
    // Then the next servicer call completes the transaction and copies results back
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_OK);
    ASSERT_EQ(txn.processing_state, HAL_I2C_TXN_STATE_COMPLETED);
    ASSERT_EQ(txn.transaction_result, HAL_I2C_TXN_RESULT_SUCCESS);
    ASSERT_EQ(txn.actual_bytes_transmitted, static_cast<size_t>(0));
    ASSERT_EQ(txn.actual_bytes_received,   static_cast<size_t>(3));
    ASSERT_EQ(txn.rx_data[0], static_cast<uint8_t>(0xA1));
    ASSERT_EQ(txn.rx_data[1], static_cast<uint8_t>(0xB2));
    ASSERT_EQ(txn.rx_data[2], static_cast<uint8_t>(0xC3));
}

TEST_F(I2CDriverTest, ISRHandlesBasicRead2Bytes)
{
    // Representing the I2C peripheral's internal shift register.
    uint32_t shift_register = 0;

    // Given an initialized I2C driver
    ASSERT_EQ(hal_i2c_init(), HAL_STATUS_OK);

    // Given a simple READ transaction of 2 bytes
    hal_i2c_txn_t txn = {};
    txn.target_addr               = 0x4F;
    txn.i2c_op                    = HAL_I2C_OP_READ;
    txn.expected_bytes_to_tx      = 0;
    txn.expected_bytes_to_rx      = 2;
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
    // Assert address was written to data register in read mode.
    ASSERT_EQ(Sim_I2C1.DR, static_cast<uint32_t>((txn.target_addr << 1) | 1)); // read = 1

    // HW-SIM: User code reads SR1 -> HW clears SB then clears START shortly after
    Sim_I2C1.SR1 &= ~I2C_SR1_SB;
    Sim_I2C1.CR1 &= ~I2C_CR1_START;

    // --------- ADDR phase ---------
    // HW-SIM: target ACKs address -> HW sets ADDR
    Sim_I2C1.SR1 |= I2C_SR1_ADDR;
    I2C1_EV_IRQHandler();
    // Assert ACK is reset for two-byte read and
    // assert POS is set (POS set means NACK the next byte) and
    // that RxNE and TxE interrupts are disabled.
    ASSERT_FALSE(Sim_I2C1.CR1 & I2C_CR1_ACK);
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_POS);
    ASSERT_FALSE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // HW-SIM: User code reads SR1 followed by SR2 -> HW clears ADDR
    Sim_I2C1.SR1 &= ~I2C_SR1_ADDR;

    // --------- RX phase ---------
    // We will deliver two data bytes: 0xD4, 0xE5

    // --- Byte #1 ---
    // HW-SIM: First byte received -> RXNE set but no iterrupt since
    // RxNE interrupts are not enabled.
    Sim_I2C1.DR  = static_cast<uint32_t>(0xD4);
    // Sim_I2C1.SR1 |= I2C_SR1_RXNE; // HW-SIM: Truthfully, the hardware sets this. But under test, I cannot
                                     // because the logic relies on hardware clearing this the moment DR is read, right in
                                     // the middle of the interrupt. I cannot simulate that in this single thread
                                     // of execution. RxNE therefore would stay set even after a DR read during this
                                     // test, which would invalidate the logic, and isn't how the hardware actually
                                     // works. See similar note just below this.

    // --- Byte #2 ---
    // HW-SIM: Next byte has arrived -> BTF and RXNE set,
    // DR contains 0xD4, Shift Register contains 0xE5.
    // Interrupt is generated by BTF.
    shift_register = static_cast<uint32_t>(0xE5);
    Sim_I2C1.SR1 |= I2C_SR1_BTF; // HW-SIM: And a little misleading. In reality, both BTF and RxNE are set right now.
                                 // But after the first read of DR in the BTF handler, the RxNE should be reset then and
                                 // there, in the middle of the interrupt. From this test suite I cannot simulate that.
                                 // Therefore, I leave RxNE reset to make sure I am testing the code paths I mean to.
    I2C1_EV_IRQHandler();

    // Driver should:
    //  - request STOP since all data has been received
    //  - reset POS
    //  - read DR -> store 0xD4
    //  - enable RxNE interrupts to catch last byte.
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_STOP);
    ASSERT_FALSE(Sim_I2C1.CR1 & I2C_CR1_POS);
    ASSERT_TRUE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // HW-SIM: Hardware clears BTF and RXNE after 0XD4 is read from DR and
    // moves 0xE5 from shift register to DR.
    Sim_I2C1.SR1 &= ~I2C_SR1_BTF;
    Sim_I2C1.SR1 &= ~I2C_SR1_RXNE;
    Sim_I2C1.CR1 &= ~I2C_CR1_STOP;
    Sim_I2C1.DR  = shift_register;

    // Moving shift register into DR sets RxNE and generates interrupt.
    Sim_I2C1.SR1 |= I2C_SR1_RXNE;
    I2C1_EV_IRQHandler();

    // Driver should disable RxNE interrupts.
    ASSERT_FALSE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // HW-SIM: Reading DR clears RxNE.
    Sim_I2C1.SR1 &= ~I2C_SR1_RXNE;

    // --------- Assert results ---------
    // Then the next servicer call completes the transaction and copies results back
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_OK);
    ASSERT_EQ(txn.processing_state, HAL_I2C_TXN_STATE_COMPLETED);
    ASSERT_EQ(txn.transaction_result, HAL_I2C_TXN_RESULT_SUCCESS);
    ASSERT_EQ(txn.actual_bytes_transmitted, static_cast<size_t>(0));
    ASSERT_EQ(txn.actual_bytes_received,   static_cast<size_t>(2));
    ASSERT_EQ(txn.rx_data[0], static_cast<uint8_t>(0xD4));
    ASSERT_EQ(txn.rx_data[1], static_cast<uint8_t>(0xE5));
}

TEST_F(I2CDriverTest, ISRHandlesBasicRead1Byte)
{
    // Given an initialized I2C driver
    ASSERT_EQ(hal_i2c_init(), HAL_STATUS_OK);

    // Given a simple READ transaction of 1 byte
    hal_i2c_txn_t txn = {};
    txn.target_addr               = 0xAA;
    txn.i2c_op                    = HAL_I2C_OP_READ;
    txn.expected_bytes_to_tx      = 0;
    txn.expected_bytes_to_rx      = 1;
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
    // Assert address was written to data register in read mode.
    ASSERT_EQ(Sim_I2C1.DR, static_cast<uint32_t>((txn.target_addr << 1) | 1)); // read = 1

    // HW-SIM: User code reads SR1 -> HW clears SB then clears START shortly after
    Sim_I2C1.SR1 &= ~I2C_SR1_SB;
    Sim_I2C1.CR1 &= ~I2C_CR1_START;

    // --------- ADDR phase ---------
    // HW-SIM: target ACKs address -> HW sets ADDR
    Sim_I2C1.SR1 |= I2C_SR1_ADDR;
    I2C1_EV_IRQHandler();
    // Assert ACK is reset for one-byte read and
    // assert STOP is requested in the case of 1 byte reception and
    // that RxNE and TxE interrupts are enabled to fire when single byte comes in.
    ASSERT_FALSE(Sim_I2C1.CR1 & I2C_CR1_ACK);
    ASSERT_TRUE(Sim_I2C1.CR1 & I2C_CR1_STOP);
    ASSERT_TRUE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // HW-SIM: User code reads SR1 followed by SR2 -> HW clears ADDR
    Sim_I2C1.SR1 &= ~I2C_SR1_ADDR;

    // --------- RX phase ---------
    // We will deliver one data byte: 0xF6,

    // --- Byte #1 ---
    // HW-SIM: First byte received -> RXNE set generates interrupt.
    Sim_I2C1.DR  = static_cast<uint32_t>(0xF6);
    Sim_I2C1.SR1 |= I2C_SR1_RXNE;
    I2C1_EV_IRQHandler();

    // RxNE interrupts should be disabled.
    ASSERT_FALSE(Sim_I2C1.CR2 & I2C_CR2_ITBUFEN);

    // HW-SIM: Hardware clears RXNE after 0XF6 is read from DR and clears STOP shortly after.
    Sim_I2C1.SR1 &= ~I2C_SR1_RXNE;
    Sim_I2C1.CR1 &= ~I2C_CR1_STOP;

    // --------- Assert results ---------
    // Then the next servicer call completes the transaction and copies results back
    ASSERT_EQ(hal_i2c_transaction_servicer(), HAL_STATUS_OK);
    ASSERT_EQ(txn.processing_state, HAL_I2C_TXN_STATE_COMPLETED);
    ASSERT_EQ(txn.transaction_result, HAL_I2C_TXN_RESULT_SUCCESS);
    ASSERT_EQ(txn.actual_bytes_transmitted, static_cast<size_t>(0));
    ASSERT_EQ(txn.actual_bytes_received,   static_cast<size_t>(1));
    ASSERT_EQ(txn.rx_data[0], static_cast<uint8_t>(0xF6));
}

TEST_F(I2CDriverTest, ISRHandlesAddressNACK)
{
    // Given an initialized I2C driver
    ASSERT_EQ(hal_i2c_init(), HAL_STATUS_OK);

    // Prepare a simple 1-byte WRITE so we can hit the address phase.
    hal_i2c_txn_t txn = {};
    txn.target_addr               = 0x2A;
    txn.i2c_op                    = HAL_I2C_OP_WRITE;
    txn.tx_data[0]                = 0xDE;
    txn.expected_bytes_to_tx      = 1;
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
