#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "uart.h"
#include "gpio.h"
#include "systick.h"

#define MAX_RX_BYTES 256
#define MAX_CMD_LENGTH 128

// Command protocol definitions
#define CMD_PREFIX "CMD:"
#define CMD_ENTER_TEST_MODE "CMD:ENTER_TEST_MODE"
#define CMD_EXIT_TEST_MODE "CMD:EXIT_TEST_MODE"
#define CMD_ECHO_TEST "CMD:ECHO:"
#define CMD_BUFFER_TEST "CMD:BUFFER_TEST:"
#define CMD_TIMING_TEST "CMD:TIMING_TEST:"
#define CMD_ERROR_TEST "CMD:ERROR_TEST:"

#define RESPONSE_OK "OK\r\n"
#define RESPONSE_ERROR "ERROR\r\n"
#define RESPONSE_READY "READY\r\n"

// Global state
static bool command_mode = false;
static uint8_t command_buffer[MAX_CMD_LENGTH];
static size_t command_index = 0;

// Function prototypes
static bool is_command(const uint8_t* data, size_t length);
static void process_command(const uint8_t* cmd, size_t length);
static void send_response(HalUart_t uart, const char* response);
static void handle_echo_command(HalUart_t uart, const uint8_t* data, size_t length);
static void handle_buffer_test(HalUart_t uart, const uint8_t* data, size_t length);
static void reset_command_buffer(void);

/**
 * @brief Check if received data is a command
 */
static bool is_command(const uint8_t* data, size_t length)
{
    if (length < strlen(CMD_PREFIX)) {
        return false;
    }

    return strncmp((const char*)data, CMD_PREFIX, strlen(CMD_PREFIX)) == 0;
}

/**
 * @brief Send response back to host
 */
static void send_response(HalUart_t uart, const char* response)
{
    hal_uart_write(uart, (const uint8_t*)response, strlen(response));
}

/**
 * @brief Reset command buffer
 */
static void reset_command_buffer(void)
{
    memset(command_buffer, 0, sizeof(command_buffer));
    command_index = 0;
}

/**
 * @brief Handle echo command
 */
static void handle_echo_command(HalUart_t uart, const uint8_t* data, size_t length)
{
    // Extract data after "CMD:ECHO:"
    const char* echo_prefix = CMD_ECHO_TEST;
    size_t prefix_len = strlen(echo_prefix);

    if (length > prefix_len) {
        // Send OK first
        send_response(uart, RESPONSE_OK);

        // Then echo the data (skip prefix and trailing \r\n)
        size_t echo_len = length - prefix_len;
        if (echo_len > 2 && data[length-2] == '\r' && data[length-1] == '\n') {
            echo_len -= 2;
        }

        hal_uart_write(uart, &data[prefix_len], echo_len);
    } else {
        send_response(uart, RESPONSE_ERROR);
    }
}

/**
 * @brief Handle buffer test command
 */
static void handle_buffer_test(HalUart_t uart, const uint8_t* data, size_t length)
{
    // For buffer test, just acknowledge
    send_response(uart, RESPONSE_OK);

    // Could implement more sophisticated buffer testing here
    // For now, just echo back a portion to test buffer limits
    if (length > 20) {
        hal_uart_write(uart, data, 20);  // Echo first 20 bytes
    }
}

/**
 * @brief Process received command
 */
static void process_command(const uint8_t* cmd, size_t length)
{
    // Null-terminate for string comparison
    char cmd_str[MAX_CMD_LENGTH];
    size_t copy_len = (length < MAX_CMD_LENGTH - 1) ? length : MAX_CMD_LENGTH - 1;
    memcpy(cmd_str, cmd, copy_len);
    cmd_str[copy_len] = '\0';

    // Remove trailing \r\n if present
    if (copy_len >= 2 && cmd_str[copy_len-2] == '\r' && cmd_str[copy_len-1] == '\n') {
        cmd_str[copy_len-2] = '\0';
        copy_len -= 2;
    }

    // Process different commands
    if (strncmp(cmd_str, CMD_ENTER_TEST_MODE, strlen(CMD_ENTER_TEST_MODE)) == 0) {
        command_mode = true;
        send_response(HAL_UART2, RESPONSE_READY);
    }
    else if (strncmp(cmd_str, CMD_EXIT_TEST_MODE, strlen(CMD_EXIT_TEST_MODE)) == 0) {
        command_mode = false;
        send_response(HAL_UART2, RESPONSE_OK);
    }
    else if (strncmp(cmd_str, CMD_ECHO_TEST, strlen(CMD_ECHO_TEST)) == 0) {
        handle_echo_command(HAL_UART2, cmd, length);
    }
    else if (strncmp(cmd_str, CMD_BUFFER_TEST, strlen(CMD_BUFFER_TEST)) == 0) {
        handle_buffer_test(HAL_UART2, cmd, length);
    }
    else {
        // Unknown command
        send_response(HAL_UART2, RESPONSE_ERROR);
    }
}

/**
 * @brief Handle command mode processing
 */
static void handle_command_mode(const uint8_t* data, size_t length)
{
    // Accumulate data until we have a complete command (ended with \r\n)
    for (size_t i = 0; i < length; i++) {
        if (command_index < MAX_CMD_LENGTH - 1) {
            command_buffer[command_index++] = data[i];

            // Check for command termination
            if (command_index >= 2 &&
                command_buffer[command_index-2] == '\r' &&
                command_buffer[command_index-1] == '\n') {

                // Process the complete command
                process_command(command_buffer, command_index);
                reset_command_buffer();
                return;
            }
        } else {
            // Buffer overflow, reset and send error
            reset_command_buffer();
            send_response(HAL_UART2, RESPONSE_ERROR);
            return;
        }
    }
}

/**
 * @brief Handle loopback mode processing
 */
static void handle_loopback_mode(const uint8_t* rx_data_uart1, size_t bytes_read_uart1,
                                 const uint8_t* rx_data_uart2, size_t bytes_read_uart2)
{
    // Check if we received a command to enter test mode
    if (bytes_read_uart1 > 0 && is_command(rx_data_uart1, bytes_read_uart1)) {
        handle_command_mode(rx_data_uart1, bytes_read_uart1);
        return;
    }

    // Normal loopback operation
    uint8_t uart1_data[MAX_RX_BYTES];
    uint8_t uart2_data[MAX_RX_BYTES];

    // Copy and process UART1 data
    memcpy(uart1_data, rx_data_uart1, bytes_read_uart1);
    size_t uart1_bytes = bytes_read_uart1;

    // Copy and process UART2 data
    memcpy(uart2_data, rx_data_uart2, bytes_read_uart2);
    size_t uart2_bytes = bytes_read_uart2;

    // Handle newline normalization for UART1 data
    if (uart1_bytes > 0 && uart1_data[uart1_bytes-1] == '\r') {
        uart1_data[uart1_bytes] = '\n';
        uart1_bytes++;
    }

    // Handle newline normalization for UART2 data
    if (uart2_bytes > 0 && uart2_data[uart2_bytes-1] == '\r') {
        uart2_data[uart2_bytes] = '\n';
        uart2_bytes++;
    }

    // Bridge the data
    if (uart1_bytes > 0) {
        hal_uart_write(HAL_UART2, uart1_data, uart1_bytes);
    }
    if (uart2_bytes > 0) {
        hal_uart_write(HAL_UART1, uart2_data, uart2_bytes);
    }
}

/**
 * @brief Entry point to the application.
 *
 * Implemented as a super loop that can operate in:
 * 1. Loopback mode (default) - bridges UART1 and UART2
 * 2. Command mode - processes test commands from host
 */
int main(void)
{
    size_t bytes_read_uart1 = 0;
    size_t bytes_read_uart2 = 0;
    uint8_t rx_data_uart1[MAX_RX_BYTES] = { 0 };
    uint8_t rx_data_uart2[MAX_RX_BYTES] = { 0 };

    // Initialize peripherals
    hal_gpio_init(NULL);
    hal_uart_init(HAL_UART1, NULL);
    hal_uart_init(HAL_UART2, NULL);

    // Initialize command buffer
    reset_command_buffer();

    // Main loop
    while (1)
    {
        hal_delay_ms(100);

        // Clear buffers and read from both UARTs
        memset(rx_data_uart1, 0, sizeof(rx_data_uart1));
        memset(rx_data_uart2, 0, sizeof(rx_data_uart2));

        bytes_read_uart1 = 0;
        bytes_read_uart2 = 0;

        hal_uart_read(HAL_UART1, rx_data_uart1, sizeof(rx_data_uart1), &bytes_read_uart1, 0);
        hal_uart_read(HAL_UART2, rx_data_uart2, sizeof(rx_data_uart2), &bytes_read_uart2, 0);

        // Process based on current mode
        if (command_mode) {
            // In command mode, process commands from UART1
            if (bytes_read_uart1 > 0) {
                handle_command_mode(rx_data_uart1, bytes_read_uart1);
            }
        } else {
            // In loopback mode, bridge the UARTs
            handle_loopback_mode(rx_data_uart1, bytes_read_uart1,
                                rx_data_uart2, bytes_read_uart2);
        }
    }

    return 0;
}
