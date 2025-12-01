/**
 * @file i2c_transaction_queue.c
 * @brief Implementation for the transaction queue.
 *
 * Copyright (c) 2025 Cory McKiel.
 * Licensed under the MIT License. See LICENSE file in the project root.
 */
#include "i2c_transaction_queue.h"

/**
 * @brief A queue that holds references to the transactions to be processed.
 *
 * Implemented as a ring buffer with no data overwrite.
 */
typedef struct {
    hal_i2c_txn_t *transactions[I2C_TRANSACTION_QUEUE_SIZE]; /*!< Array that holds all the references to transactions. */
    size_t head;                                             /*!< head points to either an empty slot (space in queue) or to tail (no space in queue). */
    size_t tail;                                             /*!< tail points to the next message to be dequeued. */
    size_t transaction_count;                                /*!< A current count of the number of transactions in the queue. */
} i2c_transaction_queue_t;

static i2c_transaction_queue_t queue = {
    .head = 0,
    .tail = 0,
    .transaction_count = 0
};

i2c_queue_status_t i2c_transaction_queue_add(hal_i2c_txn_t *txn)
{
    i2c_queue_status_t status = I2C_QUEUE_STATUS_FAIL;

    if (txn)
    {
        // Check if there is space for another message
        if (queue.transaction_count < I2C_TRANSACTION_QUEUE_SIZE)
        {
            // Queue the message
            queue.transactions[queue.head] = txn;
            queue.transactions[queue.head]->processing_state = HAL_I2C_TXN_STATE_QUEUED;

            // Increment the queue
            queue.head = (queue.head + 1) % I2C_TRANSACTION_QUEUE_SIZE;
            queue.transaction_count++;

            status = I2C_QUEUE_STATUS_SUCCESS;
        }
        else
        {
            status = I2C_QUEUE_STATUS_QUEUE_FULL;
        }
    }

    return status;
}

// Double pointer to transaction. This is because everything is pass by value in C.
// The desire is to actually set the pointer passed to this function, and to set
// a parameter, there needs to be a reference. In conclusion, this is a reference to
// a pointer type.
i2c_queue_status_t i2c_transaction_queue_get_next(hal_i2c_txn_t **txn)
{
    i2c_queue_status_t status = I2C_QUEUE_STATUS_FAIL;

    if (txn)
    {
        if (queue.transaction_count == 0)
        {
            status = I2C_QUEUE_STATUS_QUEUE_EMPTY;
        }
        else
        {
            *txn = queue.transactions[queue.tail];
            queue.tail = (queue.tail + 1) % I2C_TRANSACTION_QUEUE_SIZE;
            queue.transaction_count--;
            status = I2C_QUEUE_STATUS_SUCCESS;
        }
    }

    return status;
}

void i2c_transaction_queue_reset()
{
    queue.head = 0;
    queue.tail = 0;
    queue.transaction_count = 0;
}
