function(AddDoxygenDocs)
    find_package(Doxygen REQUIRED)

    set(DOXYFILE_IN  "${CMAKE_SOURCE_DIR}/docs/Doxyfile.in")
    set(DOXYFILE_OUT "${CMAKE_BINARY_DIR}/Doxyfile")
    set(DOXYGEN_AWESOME_DIR "${CMAKE_SOURCE_DIR}/docs/doxygen-awesome-css")

    configure_file("${DOXYFILE_IN}" "${DOXYFILE_OUT}" @ONLY)

    add_custom_target(docs
    DEPENDS coverage
    COMMAND ${DOXYGEN_EXECUTABLE} "${DOXYFILE_OUT}"
    COMMAND ${CMAKE_COMMAND} -E rm -rf "${CMAKE_BINARY_DIR}/docs/coverage"
    COMMAND ${CMAKE_COMMAND} -E rename
        "${CMAKE_BINARY_DIR}/coverage"
        "${CMAKE_BINARY_DIR}/docs/coverage"
    WORKING_DIRECTORY "${CMAKE_BINARY_DIR}"
    COMMENT "Generating API documentation with Doxygen"
    )

endfunction()
