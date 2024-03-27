add_library(ublk_sys INTERFACE
    epoll.hpp
    file.hpp
    genl.hpp
    page.hpp
)
add_library(ublk::sys ALIAS ublk_sys)

target_include_directories(ublk_sys INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/..
)

target_link_libraries(ublk_sys INTERFACE
    eve::eve
    Boost::system
    libnl::libnl
)
