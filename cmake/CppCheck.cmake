find_program(CPPCHECK_EXECUTABLE NAMES cppcheck REQUIRED)

# CPPCHECK_FILE is a generated file.
# cppcheck will analyze all sources listed inside.
set(CPPCHECK_FILE "cppcheck_files.txt")
set(CPPCHECK_FILEPATH ${CMAKE_BINARY_DIR}/${CPPCHECK_FILE})

function(AddCppCheckTarget sourceList)
    # Write the list of sources to the CPPCHECK_FILE.
    file(WRITE ${CPPCHECK_FILEPATH} "")
    foreach(src ${sourceList})
        file(APPEND ${CPPCHECK_FILEPATH} "${src}\n")
    endforeach(src ${sourceList})

    # Add the cppcheck target, providing the file describing
    # which sources to analyze.
    add_custom_target(cppcheck ALL
        COMMAND ${CPPCHECK_EXECUTABLE}
        --error-exitcode=1
        --file-list=${CPPCHECK_FILEPATH}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Running cppcheck analysis..."
    )
endfunction()
