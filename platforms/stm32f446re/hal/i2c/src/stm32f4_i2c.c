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
#include "stm32f4_hal.h"

/* Private variables */
static HalI2cStats_t i2c_stats = {0};

static void configure_gpio();

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

    if (bytes_written) {
        *bytes_written = 0;
    }

    return HAL_STATUS_ERROR;
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

    // Set PB8 (i2c1 SCL pin) moe to alternate function.
    GPIOB->MODER &= ~BIT_16;
    GPIOB->MODER |= BIT_17;

    // Set PB9 (i2c1 SDA pin) moe to alternate function.
    GPIOB->MODER &= ~BIT_18;
    GPIOB->MODER |= BIT_19;

    // Set PB8 alternate function type to I2C (AF04)
    GPIOB->AFR[1] &= ~(0xF << (PIN_0 * AF_SHIFT_WIDTH));
    GPIOB->AFR[1] |= (AF4_MASK << (PIN_0 * AF_SHIFT_WIDTH));

    // Set PB9 alternate function type to I2C (AF04)
    GPIOB->AFR[1] &= ~(0xF << (PIN_1 * AF_SHIFT_WIDTH));
    GPIOB->AFR[1] |= (AF4_MASK << (PIN_1 * AF_SHIFT_WIDTH));
}
