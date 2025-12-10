add_library(hal_interface INTERFACE)

target_include_directories(hal_interface
    INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/hal>
    $<INSTALL_INTERFACE:include>
)
