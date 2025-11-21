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

## Getting Started
