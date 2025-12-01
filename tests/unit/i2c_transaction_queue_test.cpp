#include "gtest/gtest.h"

extern "C" {
    #include "i2c_transaction_queue.h"
}

class I2CTransactionQueueTest : public ::testing::Test {
    protected:
        void SetUp() override {
            i2c_transaction_queue_reset();
        }

        void TearDown() override {

        }
};

TEST_F(I2CTransactionQueueTest, QueueAddRejectsNull)
{
    ASSERT_EQ(i2c_transaction_queue_add(nullptr), I2C_QUEUE_STATUS_FAIL);
}

TEST_F(I2CTransactionQueueTest, QueueNextRejectsNull)
{
    ASSERT_EQ(i2c_transaction_queue_get_next(nullptr), I2C_QUEUE_STATUS_FAIL);
}

TEST_F(I2CTransactionQueueTest, BasicPushPop)
{
    // The transaction to queue.
    hal_i2c_txn_t txn_in = {
        .i2c_op = HAL_I2C_OP_WRITE_READ, // Only init this.
    };

    // The handle the dequeuer would receive.
    hal_i2c_txn_t *txn_out = nullptr;

    // Assert we can add the transaction to the queue.
    ASSERT_EQ(i2c_transaction_queue_add(&txn_in), I2C_QUEUE_STATUS_SUCCESS);

    // Assert we can grab it from the queue.
    ASSERT_EQ(i2c_transaction_queue_get_next(&txn_out), I2C_QUEUE_STATUS_SUCCESS);

    // Assert our handle is no longer NULL.
    ASSERT_NE(txn_out, nullptr);

    // Assert that our data is there.
    ASSERT_EQ(txn_out->i2c_op, HAL_I2C_OP_WRITE_READ);

    // Assert we are pointing to the correct data.
    ASSERT_EQ(txn_out, &txn_in);
}

TEST_F(I2CTransactionQueueTest, ReturnsQueueFullStatus)
{
    // Define one more transaction than can fit in the queue.
    hal_i2c_txn_t transactions[I2C_TRANSACTION_QUEUE_SIZE + 1];

    // Fill the queue.
    for (int i = 0; i < I2C_TRANSACTION_QUEUE_SIZE; i++)
    {
        ASSERT_EQ(i2c_transaction_queue_add(&transactions[i]), I2C_QUEUE_STATUS_SUCCESS);
    }

    // Expect the next transaction to return QUEUE FULL
    ASSERT_EQ(i2c_transaction_queue_add(&transactions[I2C_TRANSACTION_QUEUE_SIZE]), I2C_QUEUE_STATUS_QUEUE_FULL);
}

TEST_F(I2CTransactionQueueTest, ReturnsQueueEmptyStatus)
{
    hal_i2c_txn_t *txn_out = nullptr;

    // Assert the queue is empty.
    ASSERT_EQ(i2c_transaction_queue_get_next(&txn_out), I2C_QUEUE_STATUS_QUEUE_EMPTY);
}

TEST_F(I2CTransactionQueueTest, QueueCanBeReset)
{
    hal_i2c_txn_t *txn_out = nullptr;
    hal_i2c_txn_t transactions[I2C_TRANSACTION_QUEUE_SIZE];

    // Add a couple transactions.
    for (int i = 0; i < 3 && i < I2C_TRANSACTION_QUEUE_SIZE; i++)
    {
        ASSERT_EQ(i2c_transaction_queue_add(&transactions[i]), I2C_QUEUE_STATUS_SUCCESS);
    }

    // Reset the queue.
    i2c_transaction_queue_reset();

    // Assert the queue is empty.
    ASSERT_EQ(i2c_transaction_queue_get_next(&txn_out), I2C_QUEUE_STATUS_QUEUE_EMPTY);

    // Assert txn_out is still null.
    ASSERT_EQ(txn_out, nullptr);
}

TEST_F(I2CTransactionQueueTest, QueueAddMarksTransactionAsQueued)
{
    // Setup
    hal_i2c_txn_t txn_in = {
        .processing_state = HAL_I2C_TXN_STATE_CREATED, // init the processing state to CREATED.
    };
    ASSERT_EQ(i2c_transaction_queue_add(&txn_in), I2C_QUEUE_STATUS_SUCCESS);

    // Assert the processing state is now QUEUED.
    ASSERT_EQ(txn_in.processing_state, HAL_I2C_TXN_STATE_QUEUED);
}

TEST_F(I2CTransactionQueueTest, QueueHandlesRollover)
{
    const size_t ROLLOVER_INCREMENT = (I2C_TRANSACTION_QUEUE_SIZE / 2) + 1;
    const size_t NUM_OF_TRANSACTIONS = 100 * ROLLOVER_INCREMENT; // Divisible by ROLLOVER_INCREMENT.
    size_t transaction_in_index = 0;
    size_t transaction_out_index = 0;
    static hal_i2c_txn_t transactions_in[NUM_OF_TRANSACTIONS]; // Static because this data structure can be too large for the stack.
    hal_i2c_txn_t *transaction_out = nullptr;

    // Conduct the rollover test.
    for (size_t i = 0; i < NUM_OF_TRANSACTIONS / ROLLOVER_INCREMENT; i++)
    {
        // Queue ROLLOVER_INCREMENT number of transactions.
        for (size_t j = 0; j < ROLLOVER_INCREMENT; j++)
        {
            ASSERT_EQ(i2c_transaction_queue_add(&transactions_in[transaction_in_index++]), I2C_QUEUE_STATUS_SUCCESS);
        }

        // Dequeue ROLLOVER_INCREMENT number of transactions.
        for (size_t j = 0; j < ROLLOVER_INCREMENT; j++)
        {
            // Make sure we can dequeue.
            ASSERT_EQ(i2c_transaction_queue_get_next(&transaction_out), I2C_QUEUE_STATUS_SUCCESS);

            // Assert that what we got out is what we put in and in the correct order.
            ASSERT_EQ(&transactions_in[transaction_out_index++], transaction_out);

            transaction_out = nullptr;
        }

        // Make sure these stay synced after queue/dequeue cycles.
        ASSERT_EQ(transaction_in_index, transaction_out_index);
    }
}

TEST_F(I2CTransactionQueueTest, NoFieldsAreUnexpectedlyModifiedByQueue)
{
    hal_i2c_txn_t *my_transaction_ptr = nullptr;
    hal_i2c_txn_t my_transaction = {
        // Immutable once submitted.
        .target_addr = 0x58,
        .i2c_op = HAL_I2C_OP_WRITE_READ,
        .tx_data = { 0x23 },
        .expected_bytes_to_tx = 1,
        .expected_bytes_to_rx = 1,

        // Poll to determine completion status.
        .processing_state = HAL_I2C_TXN_STATE_CREATED,

        // Post transaction completion results.
        .transaction_result = HAL_I2C_TXN_RESULT_NONE,
        .actual_bytes_received = 0,
        .actual_bytes_transmitted = 0,
        .rx_data = {0},
    };

    // Send the transaction through the queue.
    ASSERT_EQ(i2c_transaction_queue_add(&my_transaction), I2C_QUEUE_STATUS_SUCCESS);
    ASSERT_EQ(i2c_transaction_queue_get_next(&my_transaction_ptr), I2C_QUEUE_STATUS_SUCCESS);
    ASSERT_EQ(&my_transaction, my_transaction_ptr);

    // Verify nothing was unexpectedly modified.
    ASSERT_EQ(my_transaction.target_addr, 0x58);
    ASSERT_EQ(my_transaction.i2c_op, HAL_I2C_OP_WRITE_READ);
    ASSERT_EQ(my_transaction.tx_data[0], 0x23);
    ASSERT_EQ(my_transaction.expected_bytes_to_tx, 1);
    ASSERT_EQ(my_transaction.expected_bytes_to_rx, 1);
    ASSERT_EQ(my_transaction.processing_state, HAL_I2C_TXN_STATE_QUEUED); // Only one that mutates.
    ASSERT_EQ(my_transaction.transaction_result, HAL_I2C_TXN_RESULT_NONE);
    ASSERT_EQ(my_transaction.actual_bytes_received, 0);
    ASSERT_EQ(my_transaction.actual_bytes_transmitted, 0);
    ASSERT_EQ(my_transaction.rx_data[0], 0);
}
