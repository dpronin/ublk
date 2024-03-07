add_library(ublk_raid4 STATIC
    read_handler.hpp
    target.cpp
    target.hpp
    write_handler.hpp
)

target_include_directories(ublk_raid4 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_raid4 PUBLIC
    Boost::system
    ublk::utils
    ublk::raidsp
    ublk::mm
)
set_property(TARGET ublk_raid4 PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(ublk::raid4 ALIAS ublk_raid4)
