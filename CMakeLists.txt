cmake_minimum_required(VERSION 3.20)

# Build options
option(SIMULATION_BUILD "Build for desktop simulation" OFF)
option(EMBEDDED_BUILD "Build for embedded target" OFF)
option(STATIC_ANALYSIS_BUILD "Build to run static analysis" OFF)

# Ensure only one target build type is selected
if(SIMULATION_BUILD AND EMBEDDED_BUILD)
  message(FATAL_ERROR "Cannot enable both SIMULATION_BUILD and EMBEDDED_BUILD at the same time.")
endif()
if(STATIC_ANALYSIS_BUILD AND EMBEDDED_BUILD)
  message(FATAL_ERROR "Cannot enable both STATIC_ANALYSIS_BUILD and EMBEDDED_BUILD at the same time.")
endif()
if(STATIC_ANALYSIS_BUILD AND SIMULATION_BUILD)
  message(FATAL_ERROR "Cannot enable both STATIC_ANALYSIS_BUILD and SIMULATION_BUILD at the same time.")
endif()

# Default to static analysis build if nothing specified
if(NOT SIMULATION_BUILD AND NOT EMBEDDED_BUILD AND NOT STATIC_ANALYSIS_BUILD)
  message(STATUS "No build type specified -- defaulting to STATIC_ANALYSIS_BUILD")
  set(STATIC_ANALYSIS_BUILD ON)
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
  add_subdirectory(external/circular_buffer)
  include(CTest)
endif()

if(EMBEDDED_BUILD)
  message(STATUS "Building embedded target...")

  add_subdirectory(platforms)
  add_subdirectory(hal_interface)
  add_subdirectory(external/circular_buffer)

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

if(STATIC_ANALYSIS_BUILD)
  message(STATUS "Building static analysis...")

  add_subdirectory(hal_interface)
  add_subdirectory(simulations/stm32f446re/stm32f4_mocks)
  add_subdirectory(external/circular_buffer)
  add_subdirectory(platforms/stm32f446re/hal)
endif()

