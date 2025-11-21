# HAL {#mainpage}

## Overview

This project is an experiment in building a testable hardware-abstraction layer for the STM32F4 from first principles, oriented toward flight-control–class embedded systems.
It addresses the tension between development speed and rigor through automated testing, modern tooling, containerized builds, and continuous integration.

The HAL provides a small set of purpose-built drivers (UART, I2C, PWM, timers, GPIO) built around a classic embedded architecture: a super-loop execution model with interrupt-driven peripherals, no RTOS, no DMA, and strictly static memory. Its scope is intentionally narrow and includes only the primitives required for embedded control workloads, avoiding generic configuration layers, dynamic allocation, and broad peripheral coverage. The design favors simplicity, predictability, and testability over configurability or feature breadth.
