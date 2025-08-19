#include "i2c.h"

typedef enum {
    _I2C_QUEUE_STATUS_ENUM_MIN = 0,
    I2C_QUEUE_STATUS_SUCCESS = _I2C_QUEUE_STATUS_ENUM_MIN,
    I2C_QUEUE_STATUS_FAIL,
    I2C_QUEUE_STATUS_QUEUE_FULL,
    I2C_QUEUE_STATUS_QUEUE_EMPTY,
    _I2C_QUEUE_STATUS_ENUM_MAX,
} i2c_queue_status_t;

i2c_queue_status_t i2c_add_transaction_to_queue(HalI2C_Txn_t *txn);
i2c_queue_status_t i2c_get_next_transaction_from_queue(HalI2C_Txn_t **txn);
