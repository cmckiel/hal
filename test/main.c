#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "systick.h"
#include "uart.h"
#include "i2c.h"

#define MAX_RX_BYTES 1024

#define MPU_6050_ADDR 0x68
#define MPU_PWR_MGMT_1_REG 0x6B

/**
 * @brief Supports External Loopback Testing by echoing everyting received back to sender.
 */
int main(void)
{
	uint8_t loop_count = 0;

	size_t bytes_read_uart1 = 0;
	size_t bytes_read_uart2 = 0;

	size_t bytes_written_uart1 = 0;
	size_t bytes_written_uart2 = 0;

	uint8_t rx_data_uart1[MAX_RX_BYTES] = { 0 };
	uint8_t rx_data_uart2[MAX_RX_BYTES] = { 0 };

	HalI2C_Txn_t my_i2c_transaction = {
		// Immutable once submitted.
		.target_addr = MPU_6050_ADDR,
		.i2c_op = HAL_I2C_OP_WRITE_READ,
		.tx_data = {0},
		.num_of_bytes_to_tx = 0,
		.expected_bytes_to_rx = 0,

		// Poll to determine completion status.
		.processing_state = HAL_I2C_TXN_STATE_CREATED,

		// Post transaction completion results.
		.transaction_result = HAL_I2C_TXN_RESULT_NONE,
		.actual_bytes_received = 0,
		.actual_bytes_transmitted = 0,
		.rx_data = {0},
	};

	// Initialize drivers.
	hal_uart_init(HAL_UART1, NULL);
	hal_uart_init(HAL_UART2, NULL);
	hal_i2c_init(NULL);

	while (1)
	{
		hal_delay_ms(20);

		// printf("[%d]Heartbeat\n\r", loop_count++);
		// fflush(stdout);

		// Reset all data structures.
		bytes_read_uart1 = 0;
		bytes_read_uart2 = 0;

		bytes_written_uart1 = 0;
		bytes_written_uart2 = 0;

		memset(rx_data_uart1, 0, sizeof(rx_data_uart1));
		memset(rx_data_uart2, 0, sizeof(rx_data_uart2));

		// Read incoming data.
		hal_uart_read(HAL_UART1, &rx_data_uart1[0], sizeof(rx_data_uart1), &bytes_read_uart1, 0);
		hal_uart_read(HAL_UART2, &rx_data_uart2[0], sizeof(rx_data_uart2), &bytes_read_uart2, 0);

		// Echo the data back to sender.
		hal_uart_write(HAL_UART1, &rx_data_uart1[0], bytes_read_uart1, &bytes_written_uart1);
		hal_uart_write(HAL_UART2, &rx_data_uart2[0], bytes_read_uart2, &bytes_written_uart2);

		// If we have a new transaction, submit it.
		if (my_i2c_transaction.processing_state == HAL_I2C_TXN_STATE_CREATED)
		{
			// Set up transaction to read the power management register.
			my_i2c_transaction.tx_data[0] = MPU_PWR_MGMT_1_REG;
			my_i2c_transaction.num_of_bytes_to_tx = 1;
			my_i2c_transaction.expected_bytes_to_rx = 1;
			hal_i2c_submit_transaction(&my_i2c_transaction);
		}

		// If the transaction is complete and some basic expectations check out, process the data and reset.
		if (my_i2c_transaction.processing_state == HAL_I2C_TXN_STATE_COMPLETED &&
			my_i2c_transaction.actual_bytes_received == my_i2c_transaction.expected_bytes_to_rx)
		{
			// printf("pwr_mgmt reg: 0x%x\n\r", my_i2c_transaction.rx_data[0]);
			// fflush(stdout);

			// Reset our I2C transaction.
			my_i2c_transaction.processing_state = HAL_I2C_TXN_STATE_CREATED;
			my_i2c_transaction.transaction_result = HAL_I2C_TXN_RESULT_NONE;
			my_i2c_transaction.actual_bytes_received = 0;
			my_i2c_transaction.actual_bytes_transmitted = 0;
			my_i2c_transaction.rx_data[0] = 0; // Cheating for now. @todo memset()
		}
		else if (my_i2c_transaction.processing_state == HAL_I2C_TXN_STATE_COMPLETED)
		{
			// printf("I2C Transaction Failure: Bytes received not equal to expected.\n\r");
			// fflush(stdout);
		}

		hal_i2c_transaction_servicer();
	}


	return 0;
}


