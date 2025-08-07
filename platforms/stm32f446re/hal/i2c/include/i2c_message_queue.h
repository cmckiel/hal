
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

#define MAX_MESSAGE_LENGTH 1024

typedef enum {
    _I2C_QUEUE_STATUS_ENUM_MIN = 0,
    I2C_QUEUE_STATUS_SUCCESS = _I2C_QUEUE_STATUS_ENUM_MIN,
    I2C_QUEUE_STATUS_FAIL,
    I2C_QUEUE_STATUS_QUEUE_FULL,
    I2C_QUEUE_STATUS_QUEUE_EMPTY,
    _I2C_QUEUE_STATUS_ENUM_MAX,
} i2c_queue_status_t;

typedef enum {
    _I2C_MESSAGE_STATE_ENUM_MIN = 0,
    I2C_MESSAGE_STATE_CREATED = _I2C_MESSAGE_STATE_ENUM_MIN,
    I2C_MESSAGE_STATE_QUEUED,
    I2C_MESSAGE_STATE_START_CONDITION_REQUESTED,
    I2C_MESSAGE_STATE_TRANSMISSION_STARTED,
    I2C_MESSAGE_STATE_ADDRESS_ACKNOWLEDGED,
    I2C_MESSAGE_STATE_TRANSFERRING_DATA,
    I2C_MESSAGE_STATE_TRANSFER_COMPLETE,
    I2C_MESSAGE_STATE_FAULT,
    _I2C_MESSAGE_STATE_ENUM_MAX,
} i2c_message_processing_state_t;

typedef enum {
    _I2C_MESSAGE_EVENT_ENUM_MIN = 0,
    I2C_MESSAGE_EVENT_ENQUEUED = _I2C_MESSAGE_EVENT_ENUM_MIN,
    I2C_MESSAGE_EVENT_START_CONDITION_REQUESTED,
    I2C_MESSAGE_EVENT_START_CONDITION_CONFIRMED,
    I2C_MESSAGE_EVENT_ADDRESS_ACKNOWLEDGED,
    I2C_MESSAGE_EVENT_ADDRESS_ACK_FAILURE,
    I2C_MESSAGE_EVENT_TXE,
    I2C_MESSAGE_EVENT_ACK_FAILURE,
    I2C_MESSAGE_EVENT_BYTE_SENT,
    I2C_MESSAGE_EVENT_TRANSFER_COMPLETE,
    I2C_MESSAGE_EVENT_FAULT,
    _I2C_MESSAGE_EVENT_ENUM_MAX,
} i2c_message_processing_event_t;

typedef struct {
    // Core data
    uint8_t slave_addr;
    uint8_t data[MAX_MESSAGE_LENGTH];
    size_t data_len;

    // Record keeping
    size_t ack_failure_count;
    size_t bytes_sent;
    i2c_message_processing_state_t proccessing_state;
    bool message_transfer_cancelled;
} i2c_message_t;

void message_queue_init();
i2c_queue_status_t add_message_to_queue(i2c_message_t *message);
i2c_queue_status_t get_current_message_from_queue(i2c_message_t *message);
bool update_current_message_state(i2c_message_processing_event_t event);
void enqueue_event(i2c_message_processing_event_t event);
i2c_message_processing_event_t dequeue_event();
bool event_queue_is_empty();
void update_current_message();
void next_message_in_queue();
void get_failed_messages();
