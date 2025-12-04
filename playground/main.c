#include "hal_system.h"
#include "hal_metadata.h"
#include "i2c.h"
#include "pwm.h"
#include "uart.h"
#include "systick.h"

#include <string.h>
#include <stdio.h>

int main(void)
{
    const hal_metadata_t* hal_metadata = hal_get_metadata();

    hal_uart_init(HAL_UART2);

    printf("Using HAL v%s (git %s [%s], built %s)\r\n",
        hal_metadata->version_str,
        hal_metadata->git_hash,
        hal_metadata->dirty_str,
        hal_metadata->build_date);


    // Super loop
    while (1)
    {
        /* YOUR CODE HERE */
    }

    return 0;
}
