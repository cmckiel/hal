# ------- PROJECT SETTINGS -------

cmake_minimum_required(VERSION 3.20)

# Include our custom cmake modules.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake")

# Build options.
option(DESKTOP_BUILD "Build for Desktop" OFF)
option(EMBEDDED_BUILD "Build for Embedded Target" OFF)

# Ensure only one target build type is selected.
if(DESKTOP_BUILD AND EMBEDDED_BUILD)
  message(FATAL_ERROR "Cannot enable both DESKTOP_BUILD and EMBEDDED_BUILD at the same time.")
endif()

# Default to desktop build if nothing specified.
if(NOT DESKTOP_BUILD AND NOT EMBEDDED_BUILD)
  message(STATUS "No build type specified -- defaulting to DESKTOP_BUILD")
  set(DESKTOP_BUILD ON)
endif()

# Declare project.
project(hal LANGUAGES C ASM CXX)

# Set standards and configuration settings.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add core project components.
add_subdirectory(external/circular_buffer)
add_subdirectory(include)
add_subdirectory(src)

if(DESKTOP_BUILD)
  message(STATUS "Building Desktop Debug...")

  # ------- DESKTOP TESTING -------

  # Add our unit tests and mocks.
  add_subdirectory(tests/mocks)
  add_subdirectory(tests/unit)

  # Pull in GTest.
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
  )
  FetchContent_MakeAvailable(googletest)

  # ------- STATIC ANALYSIS -------

  # Gather a list of all source files to be analyzed.
  file(GLOB_RECURSE HAL_ANALYSIS_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.c
  )

  # Include cppcheck and feed it the list of sources.
  include(CppCheck)
  AddCppCheckTarget("${HAL_ANALYSIS_SOURCES}")

  # Include clang-tidy and feed it the list of sources.
  include(ClangTidy)
  AddClangTidyTarget("${HAL_ANALYSIS_SOURCES}")

  # ------- DOCUMENTATION -------

  # Document the project with Doxygen.
  include(Doxygen)
  AddDoxygenDocs()
endif() # Desktop Debug Build

if(EMBEDDED_BUILD)
  # Report build type.
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building Embedded Debug...")
  elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Building Embedded Release...")
  endif()

  # Include the Device Support Package.
  add_subdirectory(device)

  # ------- INTEGRATION TESTING -------

  # Include Target Creation Utilities.
  include(Util)
  # Construct the integration test for both debug and release.
  AddTargetExecutable("integration_test" "tests/integration/main.c")

  # ------- PLAYGROUND -------

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Construct the playground only for debug builds.
    AddTargetExecutable("playground" "playground/main.c")
  endif() # Embedded Debug Build
endif() # Embedded Build
