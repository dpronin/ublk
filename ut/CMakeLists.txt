find_package(GTest CONFIG REQUIRED)

include(CTest)

add_library(ublk_ut_helpers INTERFACE
    helpers.hpp
)
target_link_libraries(ublk_ut_helpers INTERFACE
    gtest::gtest
)
target_include_directories(ublk_ut_helpers INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

add_library(ublk::ut::helpers ALIAS ublk_ut_helpers)

add_subdirectory(raid0)
add_subdirectory(raid1)
add_subdirectory(raid4)
add_subdirectory(raid5)
