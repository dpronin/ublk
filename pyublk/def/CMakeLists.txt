find_package(liburing CONFIG REQUIRED)

add_library(ublk_def STATIC
    flush_handler.hpp
    read_handler.hpp
    target.cpp
    target.hpp
    write_handler.hpp
)

target_include_directories(ublk_def PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_def PRIVATE
    ublk::utils
    liburing::liburing
)
target_link_libraries(ublk_def PUBLIC
    Boost::system
)
set_property(TARGET ublk_def PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(ublk::def ALIAS ublk_def)
