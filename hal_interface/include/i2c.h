#ifndef _I2C_H
#define _I2C_H

#include "hal_types.h"

typedef struct {
    size_t tx_errors;
    size_t rx_errors;
    size_t bus_errors;
    size_t arbitration_lost_count;
} HalI2cStats_t;

HalStatus_t hal_i2c_init(void *config);
HalStatus_t hal_i2c_deinit(void);
HalStatus_t hal_i2c_event_servicer();
HalStatus_t hal_i2c_write(uint8_t slave_addr, const uint8_t *data, size_t len, size_t *bytes_written, uint32_t timeout_ms);
HalStatus_t hal_i2c_read(uint8_t slave_addr, uint8_t *data, size_t len, size_t *bytes_read, uint32_t timeout_ms);
HalStatus_t hal_i2c_write_read(uint8_t slave_addr, const uint8_t *write_data, size_t write_len, uint8_t *read_data, size_t read_len, size_t *bytes_read, uint32_t timeout_ms);
HalStatus_t hal_i2c_get_stats(HalI2cStats_t *stats);

#endif /* _I2C_H */
