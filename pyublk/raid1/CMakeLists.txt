add_library(ublk_raid1 INTERFACE
    read_handler.hpp
    target.hpp
    write_handler.hpp
)

target_include_directories(ublk_raid1 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_raid1 INTERFACE
    Boost::system
    ublk::utils
)
set_property(TARGET ublk_raid1 PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(ublk::raid1 ALIAS ublk_raid1)
