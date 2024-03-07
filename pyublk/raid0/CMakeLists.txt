add_library(ublk_raid0 STATIC
    read_handler.hpp
    target.cpp
    target.hpp
    write_handler.hpp
)

target_include_directories(ublk_raid0 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_raid0 PUBLIC
    Boost::system
    ublk::utils
    ublk::mm
)
set_property(TARGET ublk_raid0 PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(ublk::raid0 ALIAS ublk_raid0)
