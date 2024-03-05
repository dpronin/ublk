add_library(ublk_cache STATIC
    rw_handler.cpp
    rw_handler.hpp
    flat_lru.hpp
)

target_include_directories(ublk_cache PRIVATE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_cache PRIVATE
    Boost::headers
    Boost::system
    ublk::utils
    ublk::mm
)
set_property(TARGET ublk_cache PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(ublk::cache ALIAS ublk_cache)
