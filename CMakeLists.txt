cmake_minimum_required(VERSION 3.20)

# Build options
option(SIMULATION_BUILD "Build for desktop simulation" OFF)
option(EMBEDDED_BUILD "Build for embedded target" OFF)

# Ensure only one target build type is selected
if(SIMULATION_BUILD AND EMBEDDED_BUILD)
  message(FATAL_ERROR "Cannot enable both SIMULATION_BUILD and EMBEDDED_BUILD at the same time.")
endif()

# Default to embedded build if nothing specified
if(NOT SIMULATION_BUILD AND NOT EMBEDDED_BUILD)
  message(STATUS "No build type specified -- defaulting to EMBEDDED_BUILD")
  set(EMBEDDED_BUILD ON)
endif()

set(PROJECT_NAME "hal")

project(${PROJECT_NAME} LANGUAGES C ASM CXX)

if(SIMULATION_BUILD)
  message(STATUS "Building desktop simulation...")

  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  cmake_policy(SET CMP0135 NEW)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
  )
  FetchContent_MakeAvailable(googletest)

  add_subdirectory(simulations)
  include(CTest)
endif()

if(EMBEDDED_BUILD)
  message(STATUS "Building embedded target...")

  add_subdirectory(platforms)
  add_subdirectory(hal_interface)

  add_executable(
      ${PROJECT_NAME}.elf
      test/main.c
  )

  target_link_libraries(${PROJECT_NAME}.elf stm32f4_hal)

  add_custom_target(${PROJECT_NAME}.bin ALL
      arm-none-eabi-objcopy -O binary ${PROJECT_NAME}.elf ${PROJECT_NAME}.bin
      DEPENDS ${PROJECT_NAME}.elf
  )
endif()

add_subdirectory(external/circular_buffer)
