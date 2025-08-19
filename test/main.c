#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "systick.h"
#include "hal_system.h"
#include "uart.h"
#include "i2c.h"

#define MAX_RX_BYTES 1024

#define MPU_6050_ADDR 0x68
#define MPU_PWR_MGMT_1_REG 0x6B
#define MPU_GYRO_XOUT_H_REG 0x43
#define MPU_GYRO_WAKE 0x01
#define MPU_GYRO_SLEEP 0x40

static HalI2C_Txn_t imu_read_pwr_mode = {
	// Immutable once submitted.
	.target_addr = MPU_6050_ADDR,
	.i2c_op = HAL_I2C_OP_WRITE_READ,
	.tx_data = { MPU_PWR_MGMT_1_REG },
	.num_of_bytes_to_tx = 1,
	.expected_bytes_to_rx = 1,

	// Poll to determine completion status.
	.processing_state = HAL_I2C_TXN_STATE_CREATED,

	// Post transaction completion results.
	.transaction_result = HAL_I2C_TXN_RESULT_NONE,
	.actual_bytes_received = 0,
	.actual_bytes_transmitted = 0,
	.rx_data = {0},
};

static HalI2C_Txn_t imu_wake_gyro = {
	// Immutable once submitted.
	.target_addr = MPU_6050_ADDR,
	.i2c_op = HAL_I2C_OP_WRITE,
	.tx_data = { MPU_PWR_MGMT_1_REG, MPU_GYRO_WAKE},
	.num_of_bytes_to_tx = 2,
	.expected_bytes_to_rx = 0,

	// Poll to determine completion status.
	.processing_state = HAL_I2C_TXN_STATE_CREATED,

	// Post transaction completion results.
	.transaction_result = HAL_I2C_TXN_RESULT_NONE,
	.actual_bytes_received = 0,
	.actual_bytes_transmitted = 0,
	.rx_data = {0},
};

static HalI2C_Txn_t imu_read_gyro = {
	// Immutable once submitted.
	.target_addr = MPU_6050_ADDR,
	.i2c_op = HAL_I2C_OP_WRITE_READ,
	.tx_data = { MPU_GYRO_XOUT_H_REG },
	.num_of_bytes_to_tx = 1,
	.expected_bytes_to_rx = 6, // Burst read starting at XOUT_H

	// Poll to determine completion status.
	.processing_state = HAL_I2C_TXN_STATE_CREATED,

	// Post transaction completion results.
	.transaction_result = HAL_I2C_TXN_RESULT_NONE,
	.actual_bytes_received = 0,
	.actual_bytes_transmitted = 0,
	.rx_data = {0},
};

static HalI2C_Txn_t *current_transaction = &imu_read_pwr_mode;

void reset_i2c_transaction(HalI2C_Txn_t *txn)
{
	txn->processing_state = HAL_I2C_TXN_STATE_CREATED;
	txn->transaction_result = HAL_I2C_TXN_RESULT_NONE;
	txn->actual_bytes_received = 0;
	txn->actual_bytes_transmitted = 0;
	txn->rx_data[0] = 0; // Cheating for now. @todo memset()
	txn->rx_data[1] = 0; // Cheating for now. @todo memset()
	txn->rx_data[2] = 0; // Cheating for now. @todo memset()
	txn->rx_data[3] = 0; // Cheating for now. @todo memset()
	txn->rx_data[4] = 0; // Cheating for now. @todo memset()
	txn->rx_data[5] = 0; // Cheating for now. @todo memset()
}

/**
 * @brief Supports External Loopback Testing by echoing everyting received back to sender.
 */
int main(void)
{
	uint8_t loop_count = 0;

	float gx_dps = 0;
	float gy_dps = 0;
	float gz_dps = 0;

	size_t bytes_read_uart1 = 0;
	size_t bytes_read_uart2 = 0;

	size_t bytes_written_uart1 = 0;
	size_t bytes_written_uart2 = 0;

	uint8_t rx_data_uart1[MAX_RX_BYTES] = { 0 };
	uint8_t rx_data_uart2[MAX_RX_BYTES] = { 0 };

	// Init System.
	hal_system_init();

	// Initialize drivers.
	hal_uart_init(HAL_UART1, NULL);
	hal_uart_init(HAL_UART2, NULL);
	hal_i2c_init(NULL);

	// printf("\033[2J");   // clear screen
	// printf("\033[H");    // move cursor to 1,1

	while (1)
	{
		hal_delay_ms(20);

		// Print out the results each loop:
		// printf("\033[H");    // move cursor to 1,1
		// printf("Heartbeat: [%d]\n\r", loop_count++);
		// printf("\n\r");
		// printf("gx_dps: %.2f\n\r", gx_dps);
		// printf("gy_dps: %.2f\n\r", gy_dps);
		// printf("gz_dps: %.2f\n\r", gz_dps);

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
		if (current_transaction->processing_state == HAL_I2C_TXN_STATE_CREATED)
		{
			hal_i2c_submit_transaction(current_transaction);
		}

		// If the transaction is complete and some basic expectations check out, process the data and reset.
		if (current_transaction->processing_state == HAL_I2C_TXN_STATE_COMPLETED)
		{
			// Print out the transaction results.
			// printf("\n\r");
			// printf("expected bytes transmitted: %d\n\r", current_transaction->num_of_bytes_to_tx);
			// printf("actual bytes transmitted: %d\n\r", current_transaction->actual_bytes_transmitted);

			// printf("expected bytes received: %d\n\r", current_transaction->expected_bytes_to_rx);
			// printf("actual bytes received: %d\n\r", current_transaction->actual_bytes_received);

			// printf("transaction result: %s\n\r", current_transaction->transaction_result == HAL_I2C_TXN_RESULT_SUCCESS ? "SUCCESS" : "FAILURE");

			// Determine next transaction.
			if (current_transaction == &imu_read_pwr_mode)
			{
				// Just received the results of sleep mode.
				uint8_t pwr_mode = imu_read_pwr_mode.rx_data[0];
				if (pwr_mode == MPU_GYRO_SLEEP)
				{
					// If it is sleeping we need to wake it.
					current_transaction = &imu_wake_gyro;
				}
				else if (pwr_mode == MPU_GYRO_WAKE)
				{
					current_transaction = &imu_read_gyro;
				}
				// Reset our transaction now that it is through.
				reset_i2c_transaction(&imu_read_pwr_mode);
			}
			else if (current_transaction == &imu_wake_gyro)
			{
				// Read back what we wrote.
				current_transaction = &imu_read_pwr_mode;
				reset_i2c_transaction(&imu_wake_gyro);
			}
			else if (current_transaction == &imu_read_gyro)
			{
				if (imu_read_gyro.transaction_result == HAL_I2C_TXN_RESULT_SUCCESS)
				{
					// Update my display data.
					int16_t gx = (imu_read_gyro.rx_data[0] << 8) | imu_read_gyro.rx_data[1];
					int16_t gy = (imu_read_gyro.rx_data[2] << 8) | imu_read_gyro.rx_data[3];
					int16_t gz = (imu_read_gyro.rx_data[4] << 8) | imu_read_gyro.rx_data[5];

					gx_dps = (float)gx / 131.0f;
					gy_dps = (float)gy / 131.0f;
					gz_dps = (float)gz / 131.0f;
				}
				else
				{
					current_transaction = &imu_read_pwr_mode;
				}

				reset_i2c_transaction(&imu_read_gyro);
			}
			else
			{
				// We don't know what transaction that was, reset back to read pwr.
				reset_i2c_transaction(&imu_read_pwr_mode);
				current_transaction = &imu_read_pwr_mode;
			}
		}

		hal_i2c_transaction_servicer();

		// fflush(stdout);
	}

	return 0;
}


