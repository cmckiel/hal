#ifndef _I2C_H
#define _I2C_H

#include "hal_types.h"

#define TX_MESSAGE_MAX_LENGTH 1024
#define RX_MESSAGE_MAX_LENGTH 1024

typedef enum {
    _HAL_I2C_TXN_STATE_MIN = 0,
    HAL_I2C_TXN_STATE_CREATED = _HAL_I2C_TXN_STATE_MIN,
    HAL_I2C_TXN_STATE_QUEUED,
    HAL_I2C_TXN_STATE_PROCESSING,
    HAL_I2C_TXN_STATE_COMPLETED,
    _HAL_I2C_TXN_STATE_MAX
} HalI2C_TxnState_t;

typedef enum {
    _HAL_I2C_OP_MIN = 0,
    HAL_I2C_OP_WRITE = _HAL_I2C_OP_MIN,
    HAL_I2C_OP_READ,
    HAL_I2C_OP_WRITE_READ,
    _HAL_I2C_OP_MAX
} HalI2C_Op_t;

typedef enum {
    _HAL_I2C_TXN_RESULT_MIN = 0,
    HAL_I2C_TXN_RESULT_NONE = _HAL_I2C_TXN_RESULT_MIN,
    HAL_I2C_TXN_RESULT_SUCCESS,
    HAL_I2C_TXN_RESULT_FAIL,
    _HAL_I2C_TX_RESULT_MAX
} HalI2C_TxnResult_t;

typedef struct {
    // Immutable input. Can not change once transaction has been submitted.
    uint8_t target_addr;                     // The I2C address of the target device.
    HalI2C_Op_t i2c_op;                      // The type of transaction. i.e. READ, WRITE, or WRITE-READ.
    uint8_t tx_data[TX_MESSAGE_MAX_LENGTH];  // The data to send. Put the register addr in the first slot.
    size_t num_of_bytes_to_tx;               // The num of bytes to send the device. Include the reg addr in the count.
    size_t expected_bytes_to_rx;             // The desired number of bytes to read from the device during this transaction. Only set for READ or WRITE-READ.

    // Poll to determine when transaction has been completed.
    HalI2C_TxnState_t processing_state;      // Submit transaction with CREATED. When processing_state == COMPLETED then client can collect results. Safe to check periodically.

    // Results of the transaction. Only valid once processing_state == COMPLETED.
    // Initialize to prescribed values when submitting transaction.
    HalI2C_TxnResult_t transaction_result;   // Only valid once processing_state == COMPLETED. Contains the result of the transaction (success, fail, etc). Init to NONE.
    size_t actual_bytes_received;            // The actual number of bytes that got read during the transaction. Init to 0.
    size_t actual_bytes_transmitted;         // The actual number of bytes that got transmitted during the transaction. Init to 0.
    uint8_t rx_data[RX_MESSAGE_MAX_LENGTH];  // Data read from device will be stored here. Only valid after processing_state == COMPLETED.
                                             // Init rx_data to zeros when creating the transaction struct.
} HalI2C_Txn_t;

typedef struct {
    size_t tx_errors;
    size_t rx_errors;
    size_t bus_errors;
    size_t arbitration_lost_count;
} HalI2C_Stats_t;

hal_status_t hal_i2c_init(void *config);
hal_status_t hal_i2c_deinit(void);
hal_status_t hal_i2c_submit_transaction(HalI2C_Txn_t *txn);
hal_status_t hal_i2c_transaction_servicer();
hal_status_t hal_i2c_get_stats(HalI2C_Stats_t *stats);

#endif /* _I2C_H */
