/**
 * @file i2c_transaction_queue.h
 *
 * @brief FIFO data structure used to store handles to client I2C transaction requests.
 *
 * @note Clients are responsible for owning the memory for their transactions. This
 * queue only holds handles to the transactions so they may be processed in order.
 *
 * @warning Undefined behavior will occur if the transaction added to queue was
 * located on a function's call stack that then returns.
 */
#include "i2c.h"

// Determines the maximum number of transaction requests
// that can be queued simultaneously.
#define I2C_TRANSACTION_QUEUE_SIZE 10

// Return type indicating the result of the requested queue operation.
typedef enum {
    _I2C_QUEUE_STATUS_ENUM_MIN = 0,
    I2C_QUEUE_STATUS_SUCCESS = _I2C_QUEUE_STATUS_ENUM_MIN,
    I2C_QUEUE_STATUS_FAIL,
    I2C_QUEUE_STATUS_QUEUE_FULL,
    I2C_QUEUE_STATUS_QUEUE_EMPTY,
    _I2C_QUEUE_STATUS_ENUM_MAX,
} i2c_queue_status_t;

/**
 * @brief Add a transaction to the queue.
 *
 * @param txn A reference to the tranaction to queue.
 *
 * @warning Clients are responsible for maintaining the memory for
 * their transactions. This merely queues a handle.
 *
 * @return The status of the request. SUCCESS is the only return value indicating
 * the request was queued.
 */
i2c_queue_status_t i2c_transaction_queue_add(hal_i2c_txn_t *txn);

/**
 * @brief Get the next transaction from the queue. Removes the transaction from the queue.
 *
 * @param txn A reference to a handle type for a transaction.
 *
 * @note txn is a double pointer because the client is meant to pass in a null handle that
 * they want set to point to the next transaction to process. But to actually set a param,
 * there needs to be a reference to it, hence, the double pointer.
 *
 * @return The status of the request. SUCCESS is the only return value indicating
 * the next transaction was properly dequeued.
 */
i2c_queue_status_t i2c_transaction_queue_get_next(hal_i2c_txn_t **txn);

/**
 * @brief Resets the queue.
 *
 * It resets all internal variables that manage the queue so it will be "like new".
 */
void i2c_transaction_queue_reset();
