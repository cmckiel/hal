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
#include "i2c_message_queue.h"
#include "i2c_state_machine.h"
#include "stm32f4_hal.h"

#include <string.h>

#define SYS_FREQ_MHZ 16

#define MAX_MESSAGE_DATA_LEN sizeof(((i2c_message_t*)0)->data)

static void configure_gpio();
static void configure_peripheral();
static void configure_interrupts();

/* Private variables */
static HalI2cStats_t i2c_stats = {0};
static volatile uint8_t  _addr      = 0;
static volatile uint8_t  _data[MAX_MESSAGE_DATA_LEN];
static volatile size_t   _data_len  = 0;
static volatile size_t   _tx_pos    = 0;
static volatile uint8_t  _tx_last_written = 0; // 1 after writing final byte
static volatile bool     _tx_in_progress = false;

void I2C1_EV_IRQHandler(void)
{
    uint32_t sr1 = I2C1->SR1;  // volatile read ok

    // START bit: cleared by reading SR1 then writing DR with address
    if (sr1 & I2C_SR1_SB)
    {
        (void)I2C1->SR1;                // read to clear SB
        I2C1->DR = (_addr << 1) | 0;     // write direction = write
    }

    // Address sent/ack: clear by SR1 then SR2 read
    if (sr1 & I2C_SR1_ADDR)
    {
        (void)I2C1->SR1;
        (void)I2C1->SR2;
        I2C1->CR2 |= I2C_CR2_ITBUFEN;   // allow TXE/RXNE interrupts
    }

    // If we already wrote the last byte, wait for BTF then STOP
    if ((sr1 & I2C_SR1_BTF) && _tx_last_written)
    {
        I2C1->CR1 |= I2C_CR1_STOP;
        I2C1->CR2 &= ~I2C_CR2_ITBUFEN;  // no more buffer IRQs
        _tx_last_written = 0;
        _tx_in_progress = false;
        return;
    }

    // TXE: feed next byte if any
    if (sr1 & I2C_SR1_TXE)
    {
        if (_tx_pos < _data_len)
        {
            I2C1->DR = _data[_tx_pos++];
            if (_tx_pos == _data_len)
            {
                // we just queued the final byte; arm BTF to finish
                _tx_last_written = 1;
            }
        }
        else
        {
            // Nothing to send (e.g., zero-length write)
            I2C1->CR2 &= ~I2C_CR2_ITBUFEN;
            I2C1->CR1 |= I2C_CR1_STOP;
            _tx_in_progress = false;
        }
    }

    // todo: handle STOPF in slave mode, RXNE for reads, etc.
}

void I2C1_ER_IRQHandler()
{
    uint32_t sr1 = I2C1->SR1;  // volatile read ok

    // @todo
    if (sr1 & I2C_SR1_AF)
    {
        // The slaved failed to acknowledge either address or data.
        I2C1->SR1 &= ~I2C_SR1_AF; // Reset flag.
        // Send STOP condition
        I2C1->CR1 |= I2C_CR1_STOP;
        _tx_in_progress = false;
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

/**
 * @brief Write data to I2C slave device
 * @param slave_addr 7-bit slave address
 * @param data Pointer to data to write
 * @param len Number of bytes to write
 * @param bytes_written Pointer to store actual bytes written
 * @param timeout_ms Timeout in milliseconds
 * @return HAL_STATUS_OK on success, error code otherwise
 */
HalStatus_t hal_i2c_write(uint8_t slave_addr, const uint8_t *data, size_t len,
                          size_t *bytes_written, uint32_t timeout_ms)
{
    HalStatus_t status = HAL_STATUS_ERROR;
    i2c_queue_status_t queue_status;

    // Create an I2C message.
    size_t data_len = len > MAX_MESSAGE_DATA_LEN ? MAX_MESSAGE_DATA_LEN : len;
    i2c_message_t message = {
        .slave_addr = slave_addr,
        .data = {0},
        .data_len = data_len,
        .ack_failure_count = 0,
        .bytes_sent = 0,
        .is_new_message = true,
        .message_transfer_cancelled = false,
    };
    memcpy(message.data, data, data_len);

    queue_status = add_message_to_queue(&message);

    if (queue_status == I2C_QUEUE_STATUS_SUCCESS && bytes_written)
    {
        *bytes_written = data_len;
        status = HAL_STATUS_OK;
    }

    return status;
}

/**
 * @brief Read data from I2C slave device
 * @param slave_addr 7-bit slave address
 * @param data Pointer to buffer for received data
 * @param len Number of bytes to read
 * @param bytes_read Pointer to store actual bytes read
 * @param timeout_ms Timeout in milliseconds
 * @return HAL_STATUS_OK on success, error code otherwise
 */
HalStatus_t hal_i2c_read(uint8_t slave_addr, uint8_t *data, size_t len,
                         size_t *bytes_read, uint32_t timeout_ms)
{
    // TODO: Implement I2C read operation
    // - Send START condition
    // - Send slave address + read bit
    // - Wait for ACK
    // - Read data bytes (send ACK for all but last byte)
    // - Send NACK for last byte
    // - Send STOP condition
    // - Handle timeouts and errors

    if (bytes_read) {
        *bytes_read = 0;
    }

    return HAL_STATUS_ERROR;
}

/**
 * @brief Write then read from I2C slave device (common for register access)
 * @param slave_addr 7-bit slave address
 * @param write_data Pointer to data to write (typically register address)
 * @param write_len Number of bytes to write
 * @param read_data Pointer to buffer for received data
 * @param read_len Number of bytes to read
 * @param bytes_read Pointer to store actual bytes read
 * @param timeout_ms Timeout in milliseconds
 * @return HAL_STATUS_OK on success, error code otherwise
 */
HalStatus_t hal_i2c_write_read(uint8_t slave_addr, const uint8_t *write_data, size_t write_len,
                               uint8_t *read_data, size_t read_len, size_t *bytes_read,
                               uint32_t timeout_ms)
{
    // TODO: Implement I2C write-read operation
    // - Send START condition
    // - Send slave address + write bit
    // - Send write data (register address)
    // - Send repeated START condition
    // - Send slave address + read bit
    // - Read data bytes
    // - Send STOP condition
    // - Handle timeouts and errors

    if (bytes_read) {
        *bytes_read = 0;
    }

    return HAL_STATUS_ERROR;
}

HalStatus_t hal_i2c_event_servicer()
{
    HalStatus_t status = HAL_STATUS_ERROR;

    // CRITICAL SECTION ENTER
    NVIC_DisableIRQ(I2C1_EV_IRQn);
    NVIC_DisableIRQ(I2C1_ER_IRQn);

    // Check if I need to load in a new message for the hardware.
    if (!_tx_in_progress)
    {
        // Feed the next message.
        i2c_message_t msg;
        if (I2C_QUEUE_STATUS_SUCCESS == get_next_message(&msg))
        {
            // Message data
            _addr = msg.slave_addr;
            _data_len = msg.data_len;
            memcpy((void*)_data, msg.data, _data_len);

            // Control data
            _tx_pos = 0;
            _tx_last_written = 0;
            _tx_in_progress = true;

            // Send start
            I2C1->CR1 |= I2C_CR1_START;

            status = HAL_STATUS_OK;
        }
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
HalStatus_t hal_i2c_get_stats(HalI2cStats_t *stats)
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
