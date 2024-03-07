add_library(ublk_raidsp STATIC
    parity.cpp
    parity.hpp
    target.cpp
    target.hpp
)

target_include_directories(ublk_raidsp PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_raidsp PUBLIC
    Boost::system
    ublk::utils
    ublk::mm
)
set_property(TARGET ublk_raidsp PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(ublk::raidsp ALIAS ublk_raidsp)
