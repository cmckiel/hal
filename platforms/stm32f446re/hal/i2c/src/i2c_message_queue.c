#include "i2c_message_queue.h"

i2c_queue_status_t add_message_to_queue(i2c_message_t *message)
{
    return I2C_QUEUE_STATUS_SUCCESS;
}

i2c_queue_status_t get_current_message_from_queue(i2c_message_t *message)
{
    return I2C_QUEUE_STATUS_SUCCESS;
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

void next_message_in_queue()
{
    return;
}
