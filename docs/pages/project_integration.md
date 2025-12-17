# Project Integration

This set of instructions details how to incorporate a HAL release into an embedded project.
Assume the following steps take place in a project directory called `hal_consumer`.

## Create a CMake Project

These instructions create a CMake project capable of building an application using HAL.

### Create a Root-Level CMakeLists.txt

```cmake
# Project: Hal Consumer
cmake_minimum_required(VERSION 3.20)
project(hal-consumer C)

# Include HAL in the project.
find_package(Hal 0.1.0 REQUIRED CONFIG)

# Define the application main and link to HAL.
add_executable(hal_consumer main.c)
target_link_libraries(hal_consumer PRIVATE Hal::stm32f4_hal)

# Strip the elf and prepare a bin to flash
add_custom_target(hal_consumer_binary ALL
    arm-none-eabi-objcopy -O binary hal_consumer hal_consumer.bin
    DEPENDS hal_consumer
)
```

### Create CMakePresets.json

Add the json below to `CMakePresets.json` and swap out `<version>` for a version number like `v0.1.0`.

```json
{
    "version": 3,
    "configurePresets": [
        {
            "name": "hal-consumer",
            "displayName": "Hal Consumer (Release)",
            "description": "Build for STM32F446 target optimized",
            "binaryDir": "${sourceDir}/build",
            "toolchainFile": "${sourceDir}/CMakeToolChain.stm32f446.cmake",
            "cacheVariables": {
                "CMAKE_BUILD_TYPE": "Release",
                "CMAKE_PREFIX_PATH": "Hal-<version>-stm32f4-armv7e-m-Release"
            }
        }
    ],
    "buildPresets": [
        {
            "name": "hal-consumer",
            "displayName": "Build Hal Consumer (Release)",
            "configurePreset": "hal-consumer"
        }
    ]
}
```

### Create a CMake Toolchain File

```sh
touch CMakeToolChain.stm32f446.cmake
```

Put the following in `CMakeToolChain.stm32f446.cmake`:

```cmake
set(CMAKE_HOST_SYSTEM_NAME    Linux)
set(CMAKE_SYSTEM_NAME       Generic)
set(CMAKE_SYSTEM_PROCESSOR armv7e-m)

set(CMAKE_ASM_COMPILER arm-none-eabi-gcc)
set(CMAKE_C_COMPILER arm-none-eabi-gcc)
set(CMAKE_CXX_COMPILER arm-none-eabi-g++)
set(CMAKE_LINKER arm-none-eabi-ld)
set(CMAKE_OBJCOPY arm-none-eabi-objcopy)
set(CMAKE_OBJDUMP arm-none-eabi-objdump)
set(CMAKE_RANLIB arm-none-eabi-ranlib)
set(CMAKE_SIZE arm-none-eabi-size)
set(CMAKE_STRIP arm-none-eabi-strip)

set(CMAKE_C_FLAGS "-mcpu=cortex-m4 --specs=nano.specs -fdata-sections -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wall")
set(CMAKE_CXX_FLAGS "-mcpu=cortex-m4 --specs=nano.specs -fdata-sections -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb -Wall")
set(CMAKE_EXE_LINKER_FLAGS "-mcpu=cortex-m4 -T${PROJECT_SOURCE_DIR}/STM32F446RETX_FLASH.ld") # Path to your linker script
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} --specs=nosys.specs -Wl,-Map=\"hal.map\"")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--gc-sections -static -mfpu=fpv4-sp-d16 -mfloat-abi=hard -mthumb")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -Wl,--start-group -lc -lm -Wl,--end-group")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -u _printf_float")

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
```
## Download Release Artifacts

```sh
curl -L -O https://github.com/cmckiel/hal/releases/download/<tag>/Hal-<version>-stm32f4-armv7e-m-Release.tar.gz
```
In the above command, replace \<tag\> with the git tag associated with the version you're targeting. (e.g. `v0.1.0`) Replace \<verison\> with
the version number which typically matches the tag, (e.g. `v0.1.0`) the only exceptions being release candidates. (e.g. tag = `v0.1.0-rc3`
and version = `v0.1.0`) Note also that releases with debug symbols are available by replacing `Release` with `Debug` in the tarball name.

Next, extract the tarball into the current directory.

```sh
tar -xzf Hal-<version>-stm32f4-armv7e-m-Release.tar.gz
```

## Place Linker Script in Project Directory

Define or acquire a linker script for your board. If using the stm32f446re dev board, a linker script can be pulled down from HAL using
the following command.

```sh
curl -L -O https://raw.githubusercontent.com/cmckiel/hal/<tag>/device/linker/STM32F446RETX_FLASH.ld
```

## Create Application Main

```sh
touch main.c
```

An example can be found in @ref example-application. Note that `#include`'s differ slightly from the provided example when bringing HAL
in as a library. Just add the prefix `hal/` like so `#include "hal/uart.h"`.

## HAL Build Environment

### Pull Down Build Environment

```sh
docker pull cmckiel/hal-build-environment:latest
```

### Run the Image

```sh
docker run --rm -it -v "$PWD":/workspace -w /workspace cmckiel/hal-build-environment
```

## Build the Application

From inside the docker container run the following commands to build the target binary.

```sh
cmake --preset hal-consumer
cmake --build --preset hal-consumer
```

Success when `hal_consumer.bin` appears in the `build` dir. This will be the binary to flash to target.

See @ref deployment for information on flashing.
