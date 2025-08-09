#include "i2c_message_queue.h"
#include "circular_buffer.h"

static circular_buffer_ctx event_queue;

#define I2C_MESSAGE_QUEUE_SIZE 10

typedef struct {
    i2c_message_t messages[I2C_MESSAGE_QUEUE_SIZE];
    size_t head;
    size_t tail; // the current message
    size_t message_count;
} i2c_message_queue_t;

static i2c_message_queue_t queue = {
    .head = 0,
    .tail = 0,
    .message_count = 0
};

static void handle_ack_failure();

void message_queue_init()
{
    circular_buffer_init(&event_queue, 32);
}

i2c_queue_status_t add_message_to_queue(i2c_message_t *message)
{
    i2c_queue_status_t status = I2C_QUEUE_STATUS_FAIL;

    if (message)
    {
        // Check if there is space for another message
        if (queue.message_count < I2C_MESSAGE_QUEUE_SIZE)
        {
            // Queue the message
            queue.messages[queue.head] = *message;
            queue.messages[queue.head].is_new_message = true;

            // Increment the queue
            queue.head = (queue.head + 1) % I2C_MESSAGE_QUEUE_SIZE;
            queue.message_count++;

            status = I2C_QUEUE_STATUS_SUCCESS;
        }
        else
        {
            status = I2C_QUEUE_STATUS_QUEUE_FULL;
        }
    }

    return status;
}

i2c_queue_status_t get_next_message(i2c_message_t *message)
{
    i2c_queue_status_t status = I2C_QUEUE_STATUS_FAIL;

    if (message)
    {
        if (queue.message_count == 0)
        {
            status = I2C_QUEUE_STATUS_QUEUE_EMPTY;
        }
        else
        {
            *message = queue.messages[queue.tail];
            queue.tail = (queue.tail + 1) % I2C_MESSAGE_QUEUE_SIZE;
            queue.message_count--;
            status = I2C_QUEUE_STATUS_SUCCESS;
        }
    }

    return status;
}
