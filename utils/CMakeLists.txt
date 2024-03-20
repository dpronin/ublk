add_library(ublk_utils INTERFACE
    algo.hpp
    align.hpp
    bitset_locker.hpp
    concepts.hpp
    epoll.hpp
    file.hpp
    genl.hpp
    math.hpp
    page.hpp
    random.hpp
    size_units.hpp
    span.hpp
    utility.hpp
)
add_library(ublk::utils ALIAS ublk_utils)

target_include_directories(ublk_utils INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/..
)

target_link_libraries(ublk_utils INTERFACE
    eve::eve
    Boost::system
    libnl::libnl
)
