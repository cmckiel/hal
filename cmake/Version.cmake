function(GenerateVersionInfo)
    # Read VERSION file
    file(READ "${CMAKE_CURRENT_SOURCE_DIR}/VERSION" HAL_VERSION_STR)
    string(STRIP "${HAL_VERSION_STR}" HAL_VERSION_STR)

    # Extract x.y.z
    string(REGEX MATCH "([0-9]+)\\.([0-9]+)\\.([0-9]+)" _ "${HAL_VERSION_STR}")
    set(HAL_VERSION_MAJOR "${CMAKE_MATCH_1}")
    set(HAL_VERSION_MINOR "${CMAKE_MATCH_2}")
    set(HAL_VERSION_PATCH "${CMAKE_MATCH_3}")

    # Git hash
    execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE HAL_GIT_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    # Build date
    string(TIMESTAMP HAL_BUILD_DATE "%Y-%m-%d")

    # Generate version header
    set(GENERATED_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    file(MAKE_DIRECTORY "${GENERATED_DIR}")

    configure_file(
        "${CMAKE_CURRENT_SOURCE_DIR}/include/hal_version.h.in"
        "${GENERATED_DIR}/hal_version.h"
        @ONLY
    )
endfunction()
