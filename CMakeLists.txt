cmake_minimum_required(VERSION 3.20)

# Build options
option(DESKTOP_BUILD "Build for desktop simulation" OFF)
option(EMBEDDED_BUILD "Build for embedded target" OFF)

project(hal LANGUAGES C ASM CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

find_program(CPPCHECK_EXECUTABLE NAMES cppcheck REQUIRED)
add_subdirectory(external/circular_buffer)
add_subdirectory(include)
add_subdirectory(src)

if(DESKTOP_BUILD)
  message(STATUS "Building desktop simulation...")

  set(CMAKE_CXX_STANDARD 14)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)

  add_subdirectory(tests/mocks)
  add_subdirectory(tests/unit)

  cmake_policy(SET CMP0135 NEW)
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/heads/main.zip
  )
  FetchContent_MakeAvailable(googletest)

  include(CTest)

  add_custom_target(cppcheck ALL
      COMMAND ${CPPCHECK_EXECUTABLE}
          --error-exitcode=1
          --project=${CMAKE_BINARY_DIR}/compile_commands.json
          -i_deps
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
      COMMENT "Running cppcheck analysis..."
  )

  file(GLOB_RECURSE HAL_TIDY_SOURCES
    CONFIGURE_DEPENDS
    ${CMAKE_SOURCE_DIR}/src/*.c
  )

  # Build up a flat list of COMMAND blocks
    foreach(src ${HAL_TIDY_SOURCES})
        list(APPEND TIDY_CMDS
            COMMAND clang-tidy
                -p ${CMAKE_BINARY_DIR}
                -header-filter=${CMAKE_SOURCE_DIR}/include/hal|${CMAKE_SOURCE_DIR}/src
                ${src}
        )
    endforeach()

    # Create one target with all commands
    add_custom_target(clang-tidy
        ${TIDY_CMDS}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy on HAL sources"
        VERBATIM
    )
endif() # Desktop Debug Build

if(EMBEDDED_BUILD)
  # Include the Device Support Package
  add_subdirectory(device)

  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building embedded debug targets...")
    # Construct the integration test
    add_executable(
        ${PROJECT_NAME}_integration_test.elf
        tests/integration/main.c
    )
    target_link_libraries(
      ${PROJECT_NAME}_integration_test.elf
      PRIVATE
      stm32f4_hal
    )
    add_custom_target(${PROJECT_NAME}_integration_test.bin ALL
        arm-none-eabi-objcopy -O binary ${PROJECT_NAME}_integration_test.elf ${PROJECT_NAME}_integration_test.bin
        DEPENDS ${PROJECT_NAME}_integration_test.elf
    )

    # Construct the playground
    add_executable(
        ${PROJECT_NAME}_playground.elf
        playground/main.c
    )
    target_link_libraries(
      ${PROJECT_NAME}_playground.elf
      PRIVATE
      stm32f4_hal
    )
    add_custom_target(${PROJECT_NAME}_playground.bin ALL
        arm-none-eabi-objcopy -O binary ${PROJECT_NAME}_playground.elf ${PROJECT_NAME}_playground.bin
        DEPENDS ${PROJECT_NAME}_playground.elf
    )
  endif() # Embedded Debug Build
endif() # Embedded Build
