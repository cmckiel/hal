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

    if (NOT HAL_GIT_HASH)
        set(HAL_GIT_HASH "unknown")
    endif()

    # Determine clean/dirty
    execute_process(
        COMMAND git status --porcelain
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        OUTPUT_VARIABLE HAL_GIT_STATUS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET
        RESULT_VARIABLE HAL_GIT_STATUS_RESULT
    )

    if (HAL_GIT_STATUS_RESULT EQUAL 0)
        if (HAL_GIT_STATUS STREQUAL "")
            set(HAL_GIT_DIRTY 0)
            set(HAL_GIT_DIRTY_STR "clean")
        else()
            set(HAL_GIT_DIRTY 1)
            set(HAL_GIT_DIRTY_STR "dirty")
        endif()
    else()
        # Not a git repo or git not available
        set(HAL_GIT_DIRTY 0)
        set(HAL_GIT_DIRTY_STR "unknown")
    endif()

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
