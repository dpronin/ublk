find_package(GTest CONFIG REQUIRED)

include(CTest)

add_library(ublk_ut INTERFACE
    helpers.hpp
)
target_link_libraries(ublk_ut INTERFACE
    gtest::gtest
    ublk::mm
    ublk::utils
)
target_include_directories(ublk_ut INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

add_library(ublk::ut ALIAS ublk_ut)

add_subdirectory(cache)
add_subdirectory(inmem)
add_subdirectory(raid0)
add_subdirectory(raid1)
add_subdirectory(raid4)
add_subdirectory(raid5)
