#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#include "i2c_state_machine.h"

#define MAX_MESSAGE_LENGTH 1024

typedef enum {
    _I2C_QUEUE_STATUS_ENUM_MIN = 0,
    I2C_QUEUE_STATUS_SUCCESS = _I2C_QUEUE_STATUS_ENUM_MIN,
    I2C_QUEUE_STATUS_FAIL,
    I2C_QUEUE_STATUS_QUEUE_FULL,
    I2C_QUEUE_STATUS_QUEUE_EMPTY,
    _I2C_QUEUE_STATUS_ENUM_MAX,
} i2c_queue_status_t;

typedef struct {
    // Core data
    uint8_t slave_addr;
    uint8_t data[MAX_MESSAGE_LENGTH];
    size_t data_len;

    // Record keeping
    size_t ack_failure_count;
    size_t bytes_sent;
    bool is_new_message;
    bool message_transfer_cancelled;
} i2c_message_t;

void message_queue_init();
i2c_queue_status_t add_message_to_queue(i2c_message_t *message);
i2c_queue_status_t get_next_message(i2c_message_t *message);
void next_message_in_queue();

// Should probably separate these and move them somewhere else.
void enqueue_event(i2c_event_t event);
i2c_event_t dequeue_event();
// bool event_queue_is_empty();

// I am thinking I'll delete these..
// void update_current_message();
// void get_failed_messages();
