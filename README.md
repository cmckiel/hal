# HAL (Hardware Abstraction Layer) for STM32

> ⚠️ Work in Progress ⚠️

This project is under active development and not yet complete. Expect breaking changes and incomplete features.

## Overview

This repository contains an experimental Hardware Abstraction Layer (HAL) targeting STM32 microcontrollers. The goal is to provide a clean, modular, and maintainable abstraction layer to simplify STM32 embedded development.

## Current Status

- 🛠 Early stage design and development
- 🚧 API surface subject to change
- 🔬 Actively experimenting with architectural patterns

## Current Features

- UART2 interrupt based driver with circular buffers.
- Toggles onboard LED.
- Systick delay ms.
- UART2 Desktop hardware simulation to support unit testing the driver.
- Cmake presets for easily switching between desktop and target builds.
- Dev container for portable and consistent build environment.

## Intended Features (Planned)

- Peripheral drivers (GPIO, UART, SPI, I2C, etc.)
- Consistent, platform-agnostic APIs
- Easier portability across STM32 families
- Cleaner separation of hardware-dependent and application code

## Getting Started

More specific instructions will be provided as the project matures, but for now:
- Have docker installed
- Clone the repo.
- Use VSCode to open and build the Dev Container
- Select a CMake configure preset:
  - Embedded Target
  - Desktop Simulation (Unit tests)
- Hit the CMake `Build` button.

To deploy to target hardware:
- `st-flash write hal.bin 0x8000000 && st-flash reset`

To interact over UART2:
- `screen /dev/<your_device> 115200`
