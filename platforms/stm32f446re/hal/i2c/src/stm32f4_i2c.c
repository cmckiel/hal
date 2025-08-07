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
#include "stm32f4_hal.h"

#include <string.h>

#define SYS_FREQ_MHZ 16

#define MAX_MESSAGE_DATA_LEN sizeof(((i2c_message_t*)0)->data)

static void initiate_message_transfer();
static void conclude_message_transfer();
static void continue_message_transfer(i2c_message_t *message);
static void send_slave_address(i2c_message_t *message);
static void send_next_byte_or_resolve_message(i2c_message_t *message);
static void handle_ack_failure(i2c_message_t *message);

void I2C1_EV_IRQHandler()
{
    if (I2C1->SR1 & I2C_SR1_SB)
    {
        // The start condition has been sent on bus.
        I2C1->SR1; // Clear SB.
        enqueue_event(I2C_MESSAGE_EVENT_START_CONDITION_CONFIRMED);
    }
    else if (I2C1->SR1 & I2C_SR1_ADDR)
    {
        // The slave address has been sent and acknowledge by slave on bus.
        // Clear ADDR.
        I2C1->SR1;
        I2C1->SR2;
        enqueue_event(I2C_MESSAGE_EVENT_ADDRESS_ACKNOWLEDGED);
    }
    else if (I2C1->SR1 & I2C_SR1_TXE)
    {
        // The Transmit Data Register is ready to receive a new byte.
        enqueue_event(I2C_MESSAGE_EVENT_TXE);
    }
    else if (I2C1->SR1 & I2C_SR1_AF)
    {
        // The slaved failed to acknowledge either address or data.
        I2C1->SR1 &= ~I2C_SR1_AF; // Reset flag.
        enqueue_event(I2C_MESSAGE_EVENT_ACK_FAILURE);
    }
}

void I2C1_ER_IRQHandler()
{

}

/* Private variables */
static HalI2cStats_t i2c_stats = {0};

static void configure_gpio();
static void configure_peripheral();

/**
 * @brief Initialize I2C peripheral
 * @param config Pointer to I2C configuration structure
 * @return HAL_STATUS_OK on success, error code otherwise
 */
HalStatus_t hal_i2c_init(void *config)
{
    // TODO: Implement I2C initialization
    // - Enable I2C clock
    // - Configure I2C peripheral registers
    // - Set up interrupts if needed

    // - Configure GPIO pins for I2C (SCL/SDA)
    // These pins are broken out right next to each other on the dev board.
    // And no interference from other peripherals.
    // PB8 - I2C1 SCL
    // PB9 - I2C1 SDA
    // Need to bring up bus from port B, set these pins in AF4.
    configure_gpio();
    configure_peripheral();

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
    // TODO: Implement I2C write operation
    // - Send START condition
    // - Send slave address + write bit
    // - Wait for ACK
    // - Send data bytes
    // - Send STOP condition
    // - Handle timeouts and errors
    HalStatus_t status = HAL_STATUS_ERROR;

    // Create an I2C message.
    size_t data_len = len > MAX_MESSAGE_DATA_LEN ? MAX_MESSAGE_DATA_LEN : len;
    i2c_message_t message = {
        .slave_addr = slave_addr,
        .data = {0},
        .data_len = data_len,
        .ack_failure_count = 0,
        .bytes_sent = 0,
        .proccessing_state = I2C_MESSAGE_STATE_CREATED,
        .message_transfer_cancelled = false,
    };
    memcpy(message.data, data, data_len);

    i2c_queue_status_t queue_status;

    // CRITICAL SECTION ENTER
    queue_status = add_message_to_queue(&message);
    // CRITICAL SECTION EXIT

    if (queue_status == I2C_QUEUE_STATUS_SUCCESS && bytes_written) {
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

    i2c_message_t message;
    i2c_queue_status_t queue_status;
    queue_status = get_current_message_from_queue(&message);

    // We should make sure that we successfully retrieved a message first.
    if (queue_status == I2C_QUEUE_STATUS_SUCCESS)
    {
        // Now, I want to know if I have already started processing this one or not.
        if (message.proccessing_state == I2C_MESSAGE_STATE_QUEUED)
        {
            initiate_message_transfer();
        }
        else if (message.proccessing_state == I2C_MESSAGE_STATE_TRANSFER_COMPLETE)
        {
            conclude_message_transfer();
        }
        else
        {
            continue_message_transfer(&message);
        }

        status = HAL_STATUS_OK;
    }

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

static void initiate_message_transfer()
{
    // Send START condition
    I2C1->CR1 |= I2C_CR1_START;
    update_current_message_state(I2C_MESSAGE_EVENT_START_CONDITION_REQUESTED);
}

static void conclude_message_transfer()
{
    I2C1->CR1 |= I2C_CR1_STOP; // Send STOP
    next_message_in_queue();
}

static void continue_message_transfer(i2c_message_t *message)
{
    if (message)
    {
        i2c_message_processing_event_t event = dequeue_event();

        switch (event)
        {
            case I2C_MESSAGE_EVENT_START_CONDITION_CONFIRMED:
                send_slave_address(message);
                break;
            case I2C_MESSAGE_EVENT_ADDRESS_ACKNOWLEDGED:
                update_current_message_state(I2C_MESSAGE_EVENT_ADDRESS_ACKNOWLEDGED);
                break;
            case I2C_MESSAGE_EVENT_TXE:
                send_next_byte_or_resolve_message(message);
                break;
            case I2C_MESSAGE_EVENT_ACK_FAILURE:
                handle_ack_failure(message);
                break;
            default:
                // Something weird happened.
                update_current_message_state(I2C_MESSAGE_EVENT_FAULT);
        }
    }
}

static void send_slave_address(i2c_message_t *message)
{
    if (message)
    {
        // Send slave address + write bit (0)
        I2C1->DR = (message->slave_addr << 1) | 0;
        update_current_message_state(I2C_MESSAGE_EVENT_START_CONDITION_CONFIRMED);
    }
}

static void send_next_byte_or_resolve_message(i2c_message_t *message)
{
    if (message)
    {
        // If TXE bit is set, then the slave ack'd the data, and the hardware is ready for
        // a new byte.
        if (message->bytes_sent < message->data_len && message->bytes_sent < sizeof(message->data))
        {
            I2C1->DR = message->data[message->bytes_sent] & 0xFF;
            message->bytes_sent++;
        }
        else if (message->bytes_sent == message->data_len)
        {
            update_current_message_state(I2C_MESSAGE_EVENT_TRANSFER_COMPLETE);
        }
        else
        {
            // Bytes sent is greater than the available data, some error occurred.
            update_current_message_state(I2C_MESSAGE_EVENT_FAULT);
        }
    }
}

static void handle_ack_failure(i2c_message_t *message)
{
    // Send STOP
    I2C1->CR1 |= I2C_CR1_STOP;

    if (message)
    {
        message->ack_failure_count++;
    }

    // Retry logic
    update_current_message_state(I2C_MESSAGE_EVENT_ACK_FAILURE);
}
