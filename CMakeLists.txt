# ------- PROJECT SETTINGS -------

cmake_minimum_required(VERSION 3.20)

# Include our custom cmake modules.
list(APPEND CMAKE_MODULE_PATH "${CMAKE_SOURCE_DIR}/cmake/module")

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

# Generate the header including version and build metadata.
include(Version)
GenerateVersionInfo()

# Declare project.
project(hal
  VERSION ${PROJECT_VERSION}
  LANGUAGES C ASM CXX
)

message(STATUS "Configuring HAL v${PROJECT_VERSION}")

# Set standards and configuration settings.
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Add core project components.
add_subdirectory(external/circular_buffer)
add_subdirectory(include)
add_subdirectory(src)

if(DESKTOP_BUILD)
  message(STATUS "Building Desktop Debug")

  # ------- DESKTOP TESTING -------

  # Include CTest to allow VS Code to see unit tests.
  include(CTest)

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

  # ------- CODE COVERAGE -------

  target_compile_options(stm32f4_hal PRIVATE --coverage)
  target_link_options(stm32f4_hal PUBLIC --coverage)

  include(AddCoverage)
  AddCoverage(desktop_unit_tests)

  # ------- DOCUMENTATION -------

  # Document the project with Doxygen.
  include(Doxygen)
  AddDoxygenDocs()
endif() # Desktop Debug Build

if(EMBEDDED_BUILD)
  # Report build type.
  if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    message(STATUS "Building Embedded Debug")
  elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
    message(STATUS "Building Embedded Release")
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

  # ------- INSTALLATION -------

  # Set the installation path variable for our archive libraries.
  set(CMAKE_INSTALL_LIBDIR "lib" CACHE PATH "Install location for libraries" FORCE)

  # Pull in some helpers.
  include(GNUInstallDirs)
  include(CMakePackageConfigHelpers)

  # Install stm32f4_hal and its dependencies.
  install(TARGETS
    stm32f4_hal
    hal_interface
    circular_buffer
    stm32f4_device_support_package
    EXPORT HalTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  )

  # Install the public headers.
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/include/hal"
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  )

  # Set the place to install package config files.
  set(HAL_INSTALL_CMAKEDIR
    ${CMAKE_INSTALL_LIBDIR}/cmake/Hal
  )

  # Version file
  write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/HalConfigVersion.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMinorVersion
  )

  # Configure the main package config file from a template
  configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/template/HalConfig.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/HalConfig.cmake"
    INSTALL_DESTINATION ${HAL_INSTALL_CMAKEDIR}
  )

  # Install the export set (imported targets)
  install(EXPORT HalTargets
    NAMESPACE Hal::
    FILE HalTargets.cmake
    DESTINATION ${HAL_INSTALL_CMAKEDIR}
  )

  # Install the config and version files
  install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/HalConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/HalConfigVersion.cmake"
    DESTINATION ${HAL_INSTALL_CMAKEDIR}
  )

  # Install LICENSE and README
  install(FILES
    "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE"
    "${CMAKE_CURRENT_SOURCE_DIR}/README.md"
    DESTINATION ${CMAKE_INSTALL_DOCDIR}
  )

  # ------- PACKAGING -------

  set(CPACK_PACKAGE_NAME                "Hal")
  set(CPACK_PACKAGE_VENDOR              "Cory McKiel")
  set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "STM32F4 Hardware Abstraction Layer")

  # Set the package version
  set(CPACK_PACKAGE_VERSION       "${PROJECT_VERSION}")
  set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
  set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
  set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")

  set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_SOURCE_DIR}/LICENSE")
  set(CPACK_RESOURCE_FILE_README  "${CMAKE_CURRENT_SOURCE_DIR}/README.md")

  # Generate both tarball and zip.
  set(CPACK_GENERATOR "TGZ;ZIP")

  # Set output file name convention
  set(CPACK_PACKAGE_FILE_NAME
    "Hal-v${PROJECT_VERSION}-stm32f4-${CMAKE_SYSTEM_PROCESSOR}-${CMAKE_BUILD_TYPE}"
  )

  include(CPack)

endif() # Embedded Build
