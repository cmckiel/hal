#include "i2c_state_machine.h"

static i2c_state_t message_processing_state = I2C_STATE_TRANSMISSION_NOT_STARTED;

i2c_operation_t i2c_state_machine_get_operation(i2c_event_t event)
{
    if (message_processing_state == I2C_STATE_TRANSMISSION_NOT_STARTED)
    {
        if (event == I2C_EVENT_START_CONDITION_REQUEST)
        {
            message_processing_state = I2C_STATE_START_CONDITION_REQUESTED;
            return I2C_OP_INITIATE_TRANSFER;

        }
    }

    message_processing_state = I2C_STATE_FAULT;
    return I2C_OP_FAULT;
}

void _test_fixture_i2c_state_machine_set_state(i2c_state_t state)
{
    message_processing_state = state;
}

i2c_state_t _test_fixture_i2c_state_machine_get_state()
{
    return message_processing_state;
}
