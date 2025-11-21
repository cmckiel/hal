# HAL (Hardware Abstraction Layer) for STM32f4

A testable, first-principles hardware-abstraction layer for the STM32F4, built around a super-loop architecture with interrupt-driven drivers, static memory, and no RTOS or DMA.
Designed for flight-control-class embedded systems and developed with modern tooling, automated tests, and continuous integration.

## Current Status

### Drivers
- **UART** - Two channels, UART1 and UART2.
- **I2C** - One bus, I2C1.
- **PWM** - One channel, TIM1 (Advanced Timer).
- **Timer** - delay_ms(), Systick.
- **GPIO** - Toggle onboard LED, GPIOA.

### Test and Tooling
- **Unit Tests** - 135 unit tests with 94% code coverage.
- **Integration Test** - Hardware-in-the-Loop (HIL) integration test.
- **Static Analysis** - Two static analyzers, clang-tidy and cppcheck.
- **Containerized Build Environment** - Reproducible builds and platform independent development.
- **Continous Integration** - Automated Build, Analysis, Testing, and Deployment.

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

To see detailed build instructions, troubleshooting steps, and instructions for deploying firmware to target hardware, see the [Full Documenation](https://cmckiel.github.io/hal/).

### Example Application
```C
#include "uart.h"
#include "gpio.h"
#include "systick.h"

int main(void)
{
    hal_gpio_init(NULL);
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

## Full Documentation

> Reference the project's [Full Documenation](https://cmckiel.github.io/hal/) to learn about project scope, architecture, usage, deployment, and more.

## Project Structure

- `/include`     — HAL API
- `/src`         — Driver implementations
- /`device`      — CMSIS + vendor support
- `/tests`       — Unit and integration tests
- `/docs`        — Documentation sources
- /`playground`  - Application target for experimenting

## License

Licensed under the MIT License; see [LICENSE](https://cmckiel.github.io/hal/md__workspaces_hal_docs_license.html) for details.
