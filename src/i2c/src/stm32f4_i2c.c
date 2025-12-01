/**
 * @file stm32f4_i2c.c
 * @brief STM32F4 I2C HAL implementation
 *
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */

#ifdef DESKTOP_BUILD
#include "registers.h"
#include "nvic.h"
#else
#include "stm32f4xx.h"
#endif

#include "i2c.h"
#include "i2c_transaction_queue.h"
#include "stm32f4_hal.h"

#include <string.h>
#include <stdbool.h>

#define SYS_FREQ_MHZ 16
#define I2C_DIRECTION_WRITE 0
#define I2C_DIRECTION_READ  1

static void configure_gpio();
static void configure_peripheral();
static void configure_interrupts();
static bool load_new_transaction();
static bool current_transaction_is_valid();

/* Private variables */
static hal_i2c_txn_t   *current_i2c_transaction = NULL;

/* ISR variables */
static volatile hal_i2c_txn_t _current_i2c_transaction;
static volatile size_t       _tx_position = 0;
static volatile size_t       _rx_position = 0;
static volatile bool         _tx_last_byte_written = false;
static volatile bool         _rx_last_byte_read = false;
static volatile bool         _tx_in_progress = false;
static volatile bool         _rx_in_progress = false;
static volatile bool         _error_occured = false;

#define _SET_ERROR_FLAG_AND_ABORT_TRANSACTION() \
_error_occured = true; \
I2C1->CR2 &= ~I2C_CR2_ITBUFEN; \
I2C1->CR1 |= I2C_CR1_STOP; \
_tx_in_progress = false; \
_rx_in_progress = false;

void I2C1_EV_IRQHandler(void)
{
    // ************** START Phase **************
    // Start condition has been generated on the line.
    // Start bit has been set. It must be cleared and the
    // device address written to DR.
    // *****************************************
    if (I2C1->SR1 & I2C_SR1_SB)
    {
        // Reading SR1 clears SB.
        (void)I2C1->SR1;

        // If we're transmitting data, append the WRITE bit to address. Otherwise,
        // append the READ bit.
        if (_tx_in_progress && !_rx_in_progress)
        {
            I2C1->DR = (_current_i2c_transaction.target_addr << 1) | I2C_DIRECTION_WRITE;
        }
        else if (_rx_in_progress && !_tx_in_progress)
        {
            I2C1->DR = (_current_i2c_transaction.target_addr << 1) | I2C_DIRECTION_READ;
        }
        else
        {
            // Error with mutual exclusion of _tx_in_progress and _rx_in_progress.
            // Set error flag and bail.
            _SET_ERROR_FLAG_AND_ABORT_TRANSACTION();
        }
    }

    // ************** ADDRESS Phase **************
    // The target address has been sent on the line and
    // the target has acknowledged the address. Need to
    // set up the rest of the transaction and clear the
    // ADDR bit.
    // *****************************************
    if (I2C1->SR1 & I2C_SR1_ADDR)
    {
        // Set up ACK hardware base on reception size.
        if (_rx_in_progress)
        {
            if (_current_i2c_transaction.expected_bytes_to_rx == 1)
            {
                // Reset ACK bit so that NACK is sent on the next byte reception.
                I2C1->CR1 &= ~I2C_CR1_ACK;
            }
            else if (_current_i2c_transaction.expected_bytes_to_rx == 2)
            {
                // Reset ACK bit.
                I2C1->CR1 &= ~I2C_CR1_ACK;
                // Only set POS for 2-byte reception.
                // ACK bit controls the NACK of the next byte which is received in the shift register.
                // Therefore, the first byte will be ACK'd and the second NACK'd automatically.
                I2C1->CR1 |= I2C_CR1_POS;
            }
            else if (_current_i2c_transaction.expected_bytes_to_rx > 2)
            {
                // Set ACK bit to acknowledge received bytes until further notice.
                I2C1->CR1 |= I2C_CR1_ACK;
            }
            else
            {
                // In this scenario, an error occurred. Expected bytes to receive should be one or
                // greater if an rx is in progress.
                // Set STOP and reset ACK.
                I2C1->CR1 |= I2C_CR1_STOP;
                I2C1->CR1 &= ~I2C_CR1_ACK;
                _rx_in_progress = false;
                _tx_in_progress = false;
                _error_occured = true;
            }
        }

        // Reading SR1 followed by SR1 clears ADDR.
        // SCL stretched low until this cleared.
        (void)I2C1->SR1;
        (void)I2C1->SR2;

        // Setup STOP condition for single byte rx.
        if (_rx_in_progress && _current_i2c_transaction.expected_bytes_to_rx == 1)
        {
            // If ADDR was cleared by reading SR1 and SR2, then the clock is no longer stretched low and
            // the reception of the single byte should be happening right now as we process this instruction.
            // Stop bit needs to be set while byte is still in flight so hardware can generate STOP on time.
            I2C1->CR1 |= I2C_CR1_STOP;
            // BTF will never be set for a single byte, in which case we must enable RxNE interrupt to
            // receive our byte.
            I2C1->CR2 |= I2C_CR2_ITBUFEN;
        }

        if (_tx_in_progress)
        {
            // Enable TxE interrupts for the transmit phase.
            I2C1->CR2 |= I2C_CR2_ITBUFEN;
        }
    }

    // ************** DATA Phase **************
    // The connection has been established and the
    // hardware configured for either the tx or rx
    // phase. Need to monitor BTF, RxNE, and TxE.
    // *****************************************
    if (I2C1->SR1 & I2C_SR1_BTF)
    {
        if (_rx_in_progress)
        {
            if (_current_i2c_transaction.expected_bytes_to_rx == 2)
            {
                // For the case of 2-byte reception and BTF set, byte 1 is
                // in the DR and byte 2 is in the shift register and SCL is stretched
                // low. Set the STOP bit and then read the two bytes.
                // Reset POS for 2-byte read.
                I2C1->CR1 |= I2C_CR1_STOP;
                I2C1->CR1 &= ~I2C_CR1_POS;
                _current_i2c_transaction.rx_data[_rx_position] = I2C1->DR;
                _rx_position++;
                // Wait for the last byte to be read in the RxNE interrupt
                // to give hardware time to move it from shift register.
                I2C1->CR2 |= I2C_CR2_ITBUFEN;
            }
            else if (_current_i2c_transaction.expected_bytes_to_rx > 2)
            {
                // Count starting from 1 instead of zero indexed array.
                size_t byte_number = _rx_position + 1;
                // Assuming rx bytes numbered 1, 2, ..., N
                // If byte N-2 is in the DR, then byte N-1 is in the shift register since BTF
                // bit is set. Target is waiting to send byte N while SCL is stretch low by our micro.
                if (byte_number == (_current_i2c_transaction.expected_bytes_to_rx - 2))
                {
                    // Reset ACK bit before byte N is sent so the hardware can NACK in time.
                    I2C1->CR1 &= ~I2C_CR1_ACK;
                    // Reading the DR clears BTF and unstretches the clock. Byte N should be on
                    // its way.
                    _current_i2c_transaction.rx_data[_rx_position] = I2C1->DR;
                    _rx_position++;
                    // Arm the flag. Next BTF will mean byte N-1 in DR and N in shift register.
                    _rx_last_byte_read = true;
                }
                else if (_rx_last_byte_read)
                {
                    // Byte N-1 in DR and byte N in shift register. SCL stretched low.
                    // Time to set STOP and read last two bytes.
                    I2C1->CR1 |= I2C_CR1_STOP;

                    // Read byte N-1.
                    _current_i2c_transaction.rx_data[_rx_position] = I2C1->DR;
                    _rx_position++;
                    // Wait for the last byte to be read in the RxNE interrupt
                    // to give hardware time to move it from shift register.
                    I2C1->CR2 |= I2C_CR2_ITBUFEN;

                    // Reset the flag.
                    _rx_last_byte_read = false;
                }
                else
                {
                    // Normal read somewhere in the beginning or middle of the transaction.
                    _current_i2c_transaction.rx_data[_rx_position] = I2C1->DR;
                    _rx_position++;
                }
            }
        }
        else if (_tx_in_progress && _tx_last_byte_written)
        {
            // With BTF set during the transmit phase, and the last byte already written,
            // then both DR and shift register are empty and SCL is stretched low. Time to determine
            // whether to begin a read phase or end the transaction. Either way, the transmit phase is over.

            // Reset transmit control variables
            _tx_in_progress = false;
            _tx_last_byte_written = false;

            // Determine next phase.
            if (_current_i2c_transaction.i2c_op == HAL_I2C_OP_WRITE)
            {
                // There is no read phase. End the transaction.
                I2C1->CR1 |= I2C_CR1_STOP;
            }
            else if (_current_i2c_transaction.i2c_op == HAL_I2C_OP_WRITE_READ)
            {
                // There is a read phase. Generate a re-start.
                _rx_in_progress = true;
                I2C1->CR1 |= I2C_CR1_START;
            }

            // Clear BTF to prevent immediate refire.
            (void)I2C1->DR;
            // Disable TxE and RxNE interrupts.
            I2C1->CR2 &= ~I2C_CR2_ITBUFEN;
        }
    }

    if (I2C1->SR1 & I2C_SR1_TXE)
    {
        if (_tx_in_progress)
        {
            if (_tx_position < _current_i2c_transaction.expected_bytes_to_tx)
            {
                I2C1->DR = _current_i2c_transaction.tx_data[_tx_position];
                _tx_position++;
                if (_tx_position == _current_i2c_transaction.expected_bytes_to_tx)
                {
                    // we just queued the final byte; arm BTF to finish
                    _tx_last_byte_written = true;
                }
            }
            else if (_current_i2c_transaction.expected_bytes_to_tx == 0)
            {
                // Zero length write.
                // Disable TxE interrupt, generate STOP, and close out transaction.
                I2C1->CR2 &= ~I2C_CR2_ITBUFEN;
                I2C1->CR1 |= I2C_CR1_STOP;
                _tx_in_progress = false;
            }
        }
    }

    if (I2C1->SR1 & I2C_SR1_RXNE)
    {
        // In receive mode we only use RxNE to pick up the last byte.
        if (_rx_in_progress && _rx_position == (_current_i2c_transaction.expected_bytes_to_rx - 1))
        {
            // We are about to receive our last byte.
            _current_i2c_transaction.rx_data[_rx_position] = I2C1->DR;
            _rx_position++;

            // Turn off buffer interrupt.
            I2C1->CR2 &= ~I2C_CR2_ITBUFEN;

            // Close out the transaction.
            _rx_in_progress = false;
        }
    }
}

void I2C1_ER_IRQHandler()
{
    uint32_t sr1 = I2C1->SR1;  // volatile read ok

    if (sr1 & I2C_SR1_AF)
    {
        // The target failed to acknowledge either address or data.
        // Reset flag.
        I2C1->SR1 &= ~I2C_SR1_AF;
        _SET_ERROR_FLAG_AND_ABORT_TRANSACTION();
    }
}

hal_status_t hal_i2c_init()
{
    configure_gpio();
    configure_peripheral();
    configure_interrupts();

    return HAL_STATUS_OK;
}

hal_status_t hal_i2c_deinit()
{
    // TODO: Implement I2C deinitialization
    // - Disable I2C peripheral
    // - Disable I2C clock
    // - Reset GPIO pins
    // - Clear interrupts

    return HAL_STATUS_OK;
}

hal_status_t hal_i2c_submit_transaction(hal_i2c_txn_t *txn)
{
    // @todo: Some transaction validation here.
    return (i2c_transaction_queue_add(txn) == I2C_QUEUE_STATUS_SUCCESS) ? HAL_STATUS_OK : HAL_STATUS_ERROR;
}

hal_status_t hal_i2c_transaction_servicer()
{
    hal_status_t status = HAL_STATUS_BUSY;

    // CRITICAL SECTION ENTER
    NVIC_DisableIRQ(I2C1_EV_IRQn);
    NVIC_DisableIRQ(I2C1_ER_IRQn);

    // Check if there is currently no transaction in progress.
    if (!_tx_in_progress && !_rx_in_progress)
    {
        status = HAL_STATUS_OK;

        // Finish transaction that just completed.
        if (current_i2c_transaction)
        {
            // Transfer the results back to the client's transaction object.
            current_i2c_transaction->actual_bytes_transmitted = _tx_position;
            current_i2c_transaction->actual_bytes_received = _rx_position;
            memcpy(current_i2c_transaction->rx_data, (const void*)_current_i2c_transaction.rx_data, current_i2c_transaction->actual_bytes_received);
            current_i2c_transaction->transaction_result = (_error_occured) ? HAL_I2C_TXN_RESULT_FAIL : HAL_I2C_TXN_RESULT_SUCCESS;

            // Complete the transaction.
            current_i2c_transaction->processing_state = HAL_I2C_TXN_STATE_COMPLETED;

            // Reset our pointer away from the completed transaction.
            current_i2c_transaction = NULL;
        }

        // Load in a new transaction if there is one.
        if (load_new_transaction())
        {
            if (current_transaction_is_valid())
            {
                // Set state to processing.
                current_i2c_transaction->processing_state = HAL_I2C_TXN_STATE_PROCESSING;

                // Copy the transaction to memory that belongs to the ISR.
                _current_i2c_transaction = *current_i2c_transaction;

                // Set up the control variables.
                _error_occured = false;
                _tx_position = 0;
                _rx_position = 0;

                if (current_i2c_transaction->i2c_op == HAL_I2C_OP_WRITE ||
                    current_i2c_transaction->i2c_op == HAL_I2C_OP_WRITE_READ)
                {
                    _tx_in_progress = true;
                    _rx_in_progress = false;
                }
                else if (current_i2c_transaction->i2c_op == HAL_I2C_OP_READ)
                {
                    _tx_in_progress = false;
                    _rx_in_progress = true;
                }

                // Send start
                I2C1->CR1 |= I2C_CR1_START;
            }
            else
            {
                // Close out the invalid transaction and set error.
                status = HAL_STATUS_ERROR;
                current_i2c_transaction->actual_bytes_transmitted = 0;
                current_i2c_transaction->actual_bytes_received = 0;
                current_i2c_transaction->transaction_result = HAL_I2C_TXN_RESULT_FAIL;
                current_i2c_transaction->processing_state = HAL_I2C_TXN_STATE_COMPLETED;
                current_i2c_transaction = NULL;
            }
        }
    }

    NVIC_EnableIRQ(I2C1_EV_IRQn);
    NVIC_EnableIRQ(I2C1_ER_IRQn);
    // CRITICAL SECTION EXIT

    return status;
}

/// @brief Just for testing.
/// @warning Grave consequences if used in production code.
void _test_fixture_hal_i2c_reset_internals()
{
    current_i2c_transaction = NULL;

    _current_i2c_transaction.target_addr = 0;
    _current_i2c_transaction.i2c_op = HAL_I2C_OP_WRITE;
    _current_i2c_transaction.expected_bytes_to_tx = 0;
    _current_i2c_transaction.expected_bytes_to_rx = 0;
    _current_i2c_transaction.processing_state = HAL_I2C_TXN_STATE_CREATED;
    _current_i2c_transaction.transaction_result = HAL_I2C_TXN_RESULT_NONE;
    _current_i2c_transaction.actual_bytes_received = 0;
    _current_i2c_transaction.actual_bytes_transmitted = 0;
    memset((void*)_current_i2c_transaction.tx_data, 0, sizeof(_current_i2c_transaction.tx_data));
    memset((void*)_current_i2c_transaction.rx_data, 0, sizeof(_current_i2c_transaction.rx_data));

    _tx_position = 0;
    _rx_position = 0;
    _tx_last_byte_written = false;
    _rx_last_byte_read = false;
    _tx_in_progress = false;
    _rx_in_progress = false;
    _error_occured = false;
}

// These pins are broken out right next to each other on the dev board.
// And no interference from other peripherals.
// PB8 - I2C1 SCL
// PB9 - I2C1 SDA
// Need to bring up bus from port B, set these pins in AF4.
static void configure_gpio()
{
    // Enable Bus.
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    // Set PB8 (i2c1 SCL pin) mode to alternate function.
    GPIOB->MODER &= ~BIT_16;
    GPIOB->MODER |= BIT_17;

    // Set PB9 (i2c1 SDA pin) mode to alternate function.
    GPIOB->MODER &= ~BIT_18;
    GPIOB->MODER |= BIT_19;

    // Set PB8 alternate function type to I2C (AF04)
    GPIOB->AFR[1] &= ~(0xF << (PIN_0 * AF_SHIFT_WIDTH));
    GPIOB->AFR[1] |= (AF4_MASK << (PIN_0 * AF_SHIFT_WIDTH));

    // Set PB9 alternate function type to I2C (AF04)
    GPIOB->AFR[1] &= ~(0xF << (PIN_1 * AF_SHIFT_WIDTH));
    GPIOB->AFR[1] |= (AF4_MASK << (PIN_1 * AF_SHIFT_WIDTH));

    // Open drain
    GPIOB->OTYPER |= (GPIO_OTYPER_OT_8 | GPIO_OTYPER_OT_9);
}

static void configure_peripheral()
{
    // Send the clock to I2C1
    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;

    // Need to set the APB1 Clock frequency in the CR2 register.
    // With no dividers, it is the same as the System Frequency of 16 MHz.
    I2C1->CR2 &= ~(I2C_CR2_FREQ);
    I2C1->CR2 |= (SYS_FREQ_MHZ & I2C_CR2_FREQ);

    // Time to rise (TRISE) register. Set to 17 via the calcualtion
    // Assumed 1000 ns SCL clock rise time (maximum permitted for I2C Standard Mode)
    // Periphal's clock period (1 / SYSTEM_FREQ_MHZ)
    // (1000ns / 62.5) = 17 OR SYSTEM_FREQ_MHZ + 1 = 17. Either calc works.
    size_t trise_reg_val = SYS_FREQ_MHZ + 1;
    I2C1->TRISE &= ~(I2C_TRISE_TRISE);
    I2C1->TRISE |= (trise_reg_val & I2C_TRISE_TRISE);

    // Set CCR.
    // We want to setup CCR so that the peripheral can count up ticks of the
    // peripheral bus clock, and use that count to create the SCL clock at 100kHz for
    // Standard Mode.
    // 100 kHz SCL means 1 / 100kHz period of 10 microseconds.
    // So we need to transition the SCL clock every ~5 microseconds.
    // On a 16 MHz bus clock with a tick every 62.5 nanoseconds, this means
    // we need to transition the SCL line every 80 ticks to achieve 100kHz SCL line.
    size_t ticks_between_scl_transitions = 80;
    I2C1->CCR &= ~(I2C_CCR_CCR);
    I2C1->CCR |= (ticks_between_scl_transitions & I2C_CCR_CCR);

    // Standard mode
    I2C1->CCR &= ~I2C_CCR_FS;

    // Enable the peripheral.
    I2C1->CR1 |= I2C_CR1_PE;
}

static void configure_interrupts()
{
    I2C1->CR2 |= I2C_CR2_ITEVTEN;
    I2C1->CR2 |= I2C_CR2_ITERREN;
    NVIC_EnableIRQ(I2C1_EV_IRQn);
    NVIC_EnableIRQ(I2C1_ER_IRQn);
}

static bool current_transaction_is_valid()
{
    return (current_i2c_transaction &&
            ENUM_IN_RANGE(current_i2c_transaction->i2c_op, _HAL_I2C_OP_MIN, _HAL_I2C_OP_MAX) &&
            current_i2c_transaction->processing_state == HAL_I2C_TXN_STATE_QUEUED);
}

static bool load_new_transaction()
{
    return (I2C_QUEUE_STATUS_SUCCESS == i2c_transaction_queue_get_next(&current_i2c_transaction) &&
            current_i2c_transaction);
}
