/**
 * @file stm32f4_i2c.c
 * @brief STM32F4 I2C HAL implementation
 */

#ifdef SIMULATION_BUILD
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

/* Private variables */
static HalI2C_Stats_t i2c_stats = {0};
static HalI2C_Txn_t   *current_i2c_transaction = NULL;

/* ISR variables */
static volatile HalI2C_Txn_t _current_i2c_transaction;
static volatile size_t       _tx_position = 0;
static volatile size_t       _rx_position = 0;
static volatile bool         _tx_last_byte_written = false;
static volatile bool         _tx_in_progress = false;
static volatile bool         _rx_in_progress = false;
static volatile bool         _error_occured = false;

#define _SET_ERROR_FLAG_AND_ABORT_TRANSACTION() \
_error_occured = true; \
I2C1->CR2 &= ~I2C_CR2_ITBUFEN; \
I2C1->CR1 |= I2C_CR1_STOP; \
_tx_in_progress = false; \
_rx_in_progress = false;

#define _END_TRANSACTION() \
I2C1->CR2 &= ~I2C_CR2_ITBUFEN; \
I2C1->CR1 |= I2C_CR1_STOP; \
_tx_in_progress = false; \
_rx_in_progress = false;

void I2C1_EV_IRQHandler(void)
{
    uint32_t sr1 = I2C1->SR1;  // volatile read ok

    // START bit: cleared by reading SR1 then writing DR with address
    if (sr1 & I2C_SR1_SB)
    {
        (void)I2C1->SR1; // read to clear SB

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
        return;
    }

    // Address sent/ack: clear by SR1 then SR2 read
    if (sr1 & I2C_SR1_ADDR)
    {
        (void)I2C1->SR1;
        (void)I2C1->SR2;
        I2C1->CR2 |= I2C_CR2_ITBUFEN;   // allow TXE/RXNE interrupts
    }

    // If we already wrote the last byte, wait for BTF (Byte Transfer Finished) then STOP
    if ((sr1 & I2C_SR1_BTF) && _tx_last_byte_written)
    {
        // Transmit phase is concluded.
        _tx_in_progress = false;

        // Reset flag.
        _tx_last_byte_written = false;

        // Determine the next phase.
        if (_current_i2c_transaction.i2c_op == HAL_I2C_OP_WRITE)
        {
            // There is no READ phase. End the transaction.
            _END_TRANSACTION();
        }
        else if (_current_i2c_transaction.i2c_op == HAL_I2C_OP_WRITE_READ &&
                 _current_i2c_transaction.expected_bytes_to_rx > 0)
        {
            // Start the next transition.
            if (_current_i2c_transaction.expected_bytes_to_rx > 1)
            {
                I2C1->CR1 |= I2C_CR1_ACK; // Set up the hardware to automatically ACK each byte.
            }
            _rx_in_progress = true;
            I2C1->CR2 &= ~I2C_CR2_ITBUFEN;
            I2C1->CR1 |= I2C_CR1_START;
        }
        else
        {
            // Failure: Either the i2c_op is not WRITE or WRITE_READ, in which case we shouldn't
            // be able to get here, or we tried to WRITE_READ without specifiying an amount of
            // data to read.
            _SET_ERROR_FLAG_AND_ABORT_TRANSACTION();
        }

        return;
    }

    // TXE: feed next byte if any
    if (sr1 & I2C_SR1_TXE && _tx_in_progress && !_rx_in_progress)
    {
        if (_tx_position < _current_i2c_transaction.num_of_bytes_to_tx)
        {
            I2C1->DR = _current_i2c_transaction.tx_data[_tx_position];
            _tx_position++;
            if (_tx_position == _current_i2c_transaction.num_of_bytes_to_tx)
            {
                // we just queued the final byte; arm BTF to finish
                _tx_last_byte_written = true;
            }
        }
        else
        {
            // Nothing to send (e.g., zero-length write)
            _END_TRANSACTION();
        }
    }
    // else if (sr1 & I2C_SR1_TXE)
    // {
    //     // Error with mutual exclusion of _tx_in_progress and _rx_in_progress.
    //     // Set error flag and bail.
    //     _SET_ERROR_FLAG_AND_ABORT_TRANSACTION();
    //     return;
    // }

    if (sr1 & I2C_SR1_RXNE && _rx_in_progress && !_tx_in_progress)
    {
        if (_rx_position < _current_i2c_transaction.expected_bytes_to_rx)
        {
            _current_i2c_transaction.rx_data[_rx_position] = I2C1->DR;
            _rx_position++;

            // If there is only one byte left to send.
            if (_current_i2c_transaction.expected_bytes_to_rx - _rx_position == 1)
            {
                I2C1->CR1 &= ~I2C_CR1_ACK; // Disable ACK so we send NACK on next byte and finish transaction.
            }

            if (_rx_position == _current_i2c_transaction.expected_bytes_to_rx)
            {
                // Received our last byte. End the transaction.
                _END_TRANSACTION();
                return;
            }
        }
        else
        {
            // It appears we are trying to read without specifying how much.
            _SET_ERROR_FLAG_AND_ABORT_TRANSACTION();
            return;
        }
    }
    // else if (sr1 & I2C_SR1_RXNE)
    // {
    //     // Error with mutual exclusion of _tx_in_progress and _rx_in_progress.
    //     // Set error flag and bail.
    //     _SET_ERROR_FLAG_AND_ABORT_TRANSACTION();
    //     return;
    // }
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

/**
 * @brief Initialize I2C peripheral
 * @param config Pointer to I2C configuration structure
 * @return HAL_STATUS_OK on success, error code otherwise
 */
HalStatus_t hal_i2c_init(void *config)
{
    configure_gpio();
    configure_peripheral();
    configure_interrupts();

    return HAL_STATUS_OK;
}

/**
 * @brief Deinitialize I2C peripheral
 * @return HAL_STATUS_OK on success, error code otherwise
 */
HalStatus_t hal_i2c_deinit(void)
{
    // TODO: Implement I2C deinitialization
    // - Disable I2C peripheral
    // - Disable I2C clock
    // - Reset GPIO pins
    // - Clear interrupts

    return HAL_STATUS_OK;
}

HalStatus_t hal_i2c_submit_transaction(HalI2C_Txn_t *txn)
{
    // @todo: Some transaction validation here.
    return (i2c_transaction_queue_add(txn) == I2C_QUEUE_STATUS_SUCCESS) ? HAL_STATUS_OK : HAL_STATUS_ERROR;
}

HalStatus_t hal_i2c_transaction_servicer()
{
    HalStatus_t status = HAL_STATUS_ERROR;

    // CRITICAL SECTION ENTER
    NVIC_DisableIRQ(I2C1_EV_IRQn);
    NVIC_DisableIRQ(I2C1_ER_IRQn);

    // Check if there is currently no transaction in progress.
    if (!_tx_in_progress && !_rx_in_progress)
    {
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
        if (I2C_QUEUE_STATUS_SUCCESS == i2c_transaction_queue_get_next(&current_i2c_transaction) &&
            current_i2c_transaction)
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

        status = HAL_STATUS_OK;
    }

    NVIC_EnableIRQ(I2C1_EV_IRQn);
    NVIC_EnableIRQ(I2C1_ER_IRQn);
    // CRITICAL SECTION EXIT

    return status;
}

/**
 * @brief Get I2C statistics
 * @param stats Pointer to statistics structure to fill
 * @return HAL_STATUS_OK on success, error code otherwise
 */
HalStatus_t hal_i2c_get_stats(HalI2C_Stats_t *stats)
{
    if (!stats) {
        return HAL_STATUS_ERROR;
    }

    *stats = i2c_stats;
    return HAL_STATUS_OK;
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
