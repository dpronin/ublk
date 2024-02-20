add_library(ublk_def INTERFACE
    flush_handler.hpp
    read_handler.hpp
    target.hpp
    write_handler.hpp
)

target_include_directories(ublk_def INTERFACE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_def INTERFACE utils)
set_property(TARGET ublk_def PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(ublk::def ALIAS ublk_def)
