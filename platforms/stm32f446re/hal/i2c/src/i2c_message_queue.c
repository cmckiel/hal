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

void message_queue_init()
{
    circular_buffer_init(&event_queue, 32);
}

i2c_queue_status_t add_message_to_queue(i2c_message_t *message)
{
    i2c_queue_status_t status = I2C_QUEUE_STATUS_FAIL;

    if (message)
    {
        if (queue.message_count < I2C_MESSAGE_QUEUE_SIZE)
        {
            queue.messages[queue.head] = *message;
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

i2c_queue_status_t get_current_message_from_queue(i2c_message_t *message)
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
            status = I2C_QUEUE_STATUS_SUCCESS;
        }
    }

    return status;
}

void next_message_in_queue()
{
    if (queue.head != queue.tail)
    {
        queue.tail = (queue.tail + 1) % I2C_MESSAGE_QUEUE_SIZE;
        queue.message_count--;
    }
}

bool update_current_message_state(i2c_message_processing_event_t event)
{
    // Configurable retries?
    // if (ack_fail_count >= 3 && state == TRANSMISSION)
    // {
    //      transition to fault state. abort message transmission.
    // }
    return true;
}

void enqueue_event(i2c_message_processing_event_t event)
{
    // Safe not to implement crit section here if enqueue is only called from ISR.
    // CRITICAL SECTION START
    circular_buffer_push_no_overwrite(&event_queue, (uint8_t)event);
    // CRITICAL SECTION STOP
}

i2c_message_processing_event_t dequeue_event()
{
    uint8_t event;

    // CRITICAL SECTION START
    circular_buffer_pop(&event_queue, &event);
    // CRITICAL SECTION STOP

    return (i2c_message_processing_event_t)event;
}

bool event_queue_is_empty()
{
    // CRITICAL SECTION START
    bool res = circular_buffer_is_empty(&event_queue);
    // CRITICAL SECTION STOP

    return res;
}
