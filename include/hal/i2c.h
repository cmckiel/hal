/**
 * @file i2c.h
 * @brief Interface for the I2C module.
 *
 * The I2C module is designed around the idea of a transaction. A transaction is a
 * data structure that contains all information needed to operate on a peripheral on
 * the I2C bus (eg read register X on target, or write X to target) as well as all
 * results from the operation. The transaction is the smallest "unit" of work that the
 * I2C driver operates on. See @ref hal_i2c_txn_t for more information.
 *
 * To use the driver to communicate with a peripheral device on the I2C bus, first
 * create the transaction data struct that defines the operation and expected results.
 * Then submit the transaction to the api via reference. Maintain the memory of the
 * transaction while it is processing (the I2C module only maintains a pointer to the transaction),
 * and poll the `processing_state` to determine when the transaction is @ref HAL_I2C_TXN_STATE_COMPLETED.
 * Once completed, results can be retrieved from inside the transaction struct.
 *
 * @attention @ref hal_i2c_transaction_servicer() must be called periodically to
 * service the transactions that are submitted to the I2C driver. Otherwise, they remain in
 * the queue untouched.
 *
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#ifndef _I2C_H
#define _I2C_H

#include "hal_types.h"

/// May be set to change the size of the TX data array inside the I2C transaction data struct. Unit is bytes.
#define TX_MESSAGE_MAX_LENGTH 1024
/// May be set to change the size of the RX data array inside the I2C transaction data struct. Unit is bytes.
#define RX_MESSAGE_MAX_LENGTH 1024

/**
 * @brief The possible states of an I2C transaction (txn).
 *
 * An I2C transaction can be in only one of these states at any given
 * time. It may only progress linearly through each state, unless it fails.
 * In the case of failure, the transaction state is considered COMPLETED.
 *
 * @note The user never modifies the state manually except to set @ref HAL_I2C_TXN_STATE_CREATED
 * during transaction initialization.
 */
typedef enum {
    _HAL_I2C_TXN_STATE_MIN = 0,                         /*!< Lower bound of enum. Inclusive. */
    HAL_I2C_TXN_STATE_CREATED = _HAL_I2C_TXN_STATE_MIN, /*!< The transaction has just been created and the state is not set. */
    HAL_I2C_TXN_STATE_QUEUED,                           /*!< The transaction has been submitted to the processing queue. */
    HAL_I2C_TXN_STATE_PROCESSING,                       /*!< The transaction is now processing. It is currently commanding the I2C bus. */
    HAL_I2C_TXN_STATE_COMPLETED,                        /*!< The transaction is done being processed. Success/Failure is determined by checking @ref hal_i2c_txn_result_t. */
    _HAL_I2C_TXN_STATE_MAX                              /*!< Upper bound of enum. Exclusive. */
} hal_i2c_txn_state_t;

/**
 * @brief The common I2C operations read, write, and write-read.
 *
 * The most common use is most likely @ref HAL_I2C_OP_WRITE_READ. The mode allows reading
 * a given register on the target device by first *writing* the address of the register to
 * target follow by a *read*.
 */
typedef enum {
    _HAL_I2C_OP_MIN = 0,                /*!< Lower bound of enum. Inclusive. */
    HAL_I2C_OP_WRITE = _HAL_I2C_OP_MIN, /*!< Send bytes to the target device. */
    HAL_I2C_OP_READ,                    /*!< Read bytes from the target device. */
    HAL_I2C_OP_WRITE_READ,              /*!< Most common pattern for reading a specific register from the target device. */
    _HAL_I2C_OP_MAX                     /*!< Upper bound of enum. Exclusive. */
} hal_i2c_op_t;

/**
 * @brief Result status of a completed transaction.
 *
 * After a transaction is completed, the transaction result can be
 * checked to inquire if the transaction was completed succesfully or not.
 *
 * @note The user never modifies this except to initialize to @ref HAL_I2C_TXN_RESULT_NONE.
 */
typedef enum {
    _HAL_I2C_TXN_RESULT_MIN = 0,                       /*!< Lower bound of enum. Inclusive. */
    HAL_I2C_TXN_RESULT_NONE = _HAL_I2C_TXN_RESULT_MIN, /*!< Default. Transaction result has not been set. */
    HAL_I2C_TXN_RESULT_SUCCESS,                        /*!< The transaction was successfully processed. */
    HAL_I2C_TXN_RESULT_FAIL,                           /*!< The transaction was unabled to be processed. */
    _HAL_I2C_TX_RESULT_MAX                             /*!< Upper bound of enum. Exclusive. */
} hal_i2c_txn_result_t;

/**
 * @brief The main data structure for interacting with the I2C module.
 *
 * All interactions with the I2C driver involve using this data structure. In it is defined
 * everything needed to initiate and complete a single I2C transaction with a target device,
 * including the device address, the operation mode, the data to send (if any), and the expected
 * data to receive (if any). It also includes status information used to track progress and
 * check for success or failure.
 *
 * The use of this transaction struct involves initializing the proper inputs, polling the
 * the @ref processing_state, and reading the outputs once the state is complete.
 *
 * @note When submitting a transaction for processing, the client must maintain the memory for the transaction.
 * Only a reference is passed to the driver. If the memory for the transaction is erased (i.e. declared in a function
 * followed by the function's return) then the I2C driver will operate on a reference to invalid memory.
 */
typedef struct {
    // Immutable input. Can not change once transaction has been submitted.
    uint8_t target_addr;                     /*!< The I2C address of the target device. */
    hal_i2c_op_t i2c_op;                     /*!< The type of transaction. i.e. READ, WRITE, or WRITE-READ. */
    uint8_t tx_data[TX_MESSAGE_MAX_LENGTH];  /*!< The data to send. Put the register addr in the first slot. */
    size_t expected_bytes_to_tx;             /*!< The num of bytes to send the device. Include the reg addr in the count. */
    size_t expected_bytes_to_rx;             /*!< The desired number of bytes to read from the device during this transaction. Only set for READ or WRITE-READ. */

    // Poll to determine when transaction has been completed.
    hal_i2c_txn_state_t processing_state;    /*!< Submit transaction with CREATED. When processing_state == COMPLETED then client can collect results. Safe to check periodically.
                                                  Poll to determine when transaction has been completed. */

    // Results of the transaction. Only valid once processing_state == COMPLETED.
    // Initialize to prescribed values when submitting transaction.
    hal_i2c_txn_result_t transaction_result; /*!< Only valid once processing_state == COMPLETED. Contains the result of the transaction (success, fail, etc). Init to NONE. */
    size_t actual_bytes_received;            /*!< The actual number of bytes that got read during the transaction. Init to 0. */
    size_t actual_bytes_transmitted;         /*!< The actual number of bytes that got transmitted during the transaction. Init to 0. */
    uint8_t rx_data[RX_MESSAGE_MAX_LENGTH];  /*!< Data read from device will be stored here. Only valid after processing_state == COMPLETED.
                                                  Init rx_data to zeros when creating the transaction struct. */
} hal_i2c_txn_t;

/**
 * @brief Initialize the I2C Module. Must be called prior to using the I2C Module.
 *
 * @return @ref HAL_STATUS_OK on success.
 */
hal_status_t hal_i2c_init();

/**
 * @brief Deinitialize the I2C Module.
 *
 * @return @ref HAL_STATUS_OK on success.
 *
 * @note Will bring down the peripheral, but not the gpio port. The gpio ports
 * are shared so it is dangerous for one module to bring it down without consideration.
 */
hal_status_t hal_i2c_deinit();

/**
 * @brief Submit a transaction to be processed by the driver.
 *
 * @param txn A reference to a transaction with the appropriate fields already filled out.
 * See @ref hal_i2c_txn_t for details about filling out the transaction correctly, as well as
 * information about checking progress.
 *
 * @return @ref HAL_STATUS_OK if transaction was submitted successfully.
 *
 * @note Successful return from this function *only* represents that the transaction was
 * successfully queued.
 *
 * @note Clients must maintain the memory of their transactions. Only a *reference* to the
 * transaction is queued.
 */
hal_status_t hal_i2c_submit_transaction(hal_i2c_txn_t *txn);

/**
 * @brief Called periodically to manage the loading and unloading of
 * transactions.
 *
 * @return @ref HAL_STATUS_OK on success.
 */
hal_status_t hal_i2c_transaction_servicer();

#endif /* _I2C_H */
