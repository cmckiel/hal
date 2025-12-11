function(AddClangTidyTarget sourceList)
  # Build up a list of individual clang-tidy commands, one for each file to be analyzed.
  foreach(src ${sourceList})
    list(APPEND TIDY_CMDS
      COMMAND clang-tidy
        -p ${CMAKE_BINARY_DIR}
        -header-filter=${CMAKE_SOURCE_DIR}/include/hal|${CMAKE_SOURCE_DIR}/src
        ${src}
    )
    endforeach()

    # Create one target with all clang-tidy commands
    add_custom_target(clang-tidy
        ${TIDY_CMDS}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running clang-tidy on HAL sources"
        VERBATIM
    )
endfunction()
