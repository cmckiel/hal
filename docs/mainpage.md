# HAL {#mainpage}

## Overview

This project is an experiment in building a testable hardware-abstraction layer for the STM32F4 from first principles, oriented toward flight-control–class embedded systems.
It addresses the tension between development speed and rigor through automated testing, modern tooling, containerized builds, and continuous integration.

The HAL provides a small set of purpose-built drivers (UART, I2C, PWM, timers, GPIO) built around a classic embedded architecture: a super-loop execution model with interrupt-driven peripherals, no RTOS, no DMA, and strictly static memory. Its scope is intentionally narrow and includes only the primitives required for embedded control workloads, avoiding generic configuration layers, dynamic allocation, and broad peripheral coverage. The design favors simplicity, predictability, and testability over configurability or feature breadth.

## Architecture

<p align="center">
  <img src="HalArchitecture.svg" width="600px">
</p>

### Layer Descriptions
- **Applications** - Logic executed in the project’s super-loop (`main.c`).
- **Hardware Abstration Layer** - Public interfaces in `include/hal`.
- **STM32F4 Peripheral Drivers** - Implementations in `src` that operate directly on device registers.
- **Device Support Package** - CMSIS and vendor headers plus startup code and the linker script in `device`.
- **STM32F4 Hardware** - The STM32F446RE development board.

For more detailed architecture, see [Detailed Architecture](architecture.md).

## Quick Start

### Prerequisites
- A `UNIX-style command line` environment.
- Have `git` installed.
- Have `docker` installed

### Clone
```console
$ git clone git@github.com:cmckiel/hal.git && cd hal
$ git submodule update --init
```

### Build
First, from inside the repo directory, build the docker image:
```console
$ docker build -t hal-build-env .
```

Next, run the docker image, mounting the repo dir. This command launches an interactive shell inside the container. (Commands executed inside the container are distinguished by `#`)
```console
$ docker run --rm -it -v "$PWD":/workspace -w /workspace hal-build-env
```
> If needed: exit the docker container by pressing `ctrl-p` followed quickly by `ctrl-q`.

Build for desktop:
```console
# cmake --preset desktop-debug
# cmake --build --preset desktop-debug
```

Run the unit tests:
```console
# ctest --preset desktop-debug
```

Build for target hardware:
```console
# cmake --preset embedded-debug
# cmake --build --preset embedded-debug
```

Target firmware should be visible in `build/embedded-debug/` as `.elf` and `.bin` files.

### Example Application
```C
#include "uart.h"
#include "gpio.h"
#include "systick.h"

int main(void)
{
    hal_gpio_init();
    hal_uart_init(HAL_UART2, NULL);

    uint8_t message[] = "Hello from HAL!";
    size_t bytes_successfully_writen = 0;

    // Print hello message over serial.
    HalStatus_t status = hal_uart_write(HAL_UART2, message, sizeof(message), &bytes_successfully_writen);

    // Easy error checking.
    if (status != HAL_STATUS_OK || bytes_successfully_writen != sizeof(message))
    {
        // Transmission error handling.
        return 1;
    }

    // Super loop
    while (1)
    {
        /* APPLICATION CODE HERE */

        // Example application:
        // Toggle onboard LED at 10 Hz.
        hal_delay_ms(100);
        hal_gpio_toggle_led();
    }

    return 0;
}
```

## License

Licensed under the MIT License; see [LICENSE](license.md) for details.
