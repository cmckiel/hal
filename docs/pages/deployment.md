# Firmware Deployment

This page documents the minimal steps required to flash a pre-built firmware image to an STM32F4 target and view its serial output. Instructions assume Linux or macOS. Windows users should SSH into a Linux machine connected to the hardware.

## Prerequisites

* Linux or macOS host
* ST-Link connected to the STM32F4
* `stlink` tools installed

  * Linux: `sudo apt install stlink-tools`
  * macOS: `brew install stlink`

## Verify Device Detection

```sh
st-info --probe
```

You should see device ID and memory information.

## Flash the Firmware

Assuming your build produces `build/firmware.bin`:

```sh
st-flash write build/firmware.bin 0x08000000
st-flash reset
```

If flashing fails, confirm the board is powered and the SWD connection is correct.

## View Serial Output

Serial device names vary by platform:

* Linux: `/dev/ttyACM0`, `/dev/ttyUSB0`
* macOS: `/dev/cu.usbmodem*`, `/dev/cu.usbserial*`

Identify the device by comparing `ls /dev/tty*` before and after plugging in.

Example:

```sh
screen /dev/ttyACM0 115200
```

Exit screen with `Ctrl-A` then `k` then `y`.

## Windows Notes

Windows workflows are not formally supported. Use:

* SSH into a Linux host connected to the board; or
* WSL2 with USB/IP passthrough (advanced).
