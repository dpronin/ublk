find_package(GTest CONFIG REQUIRED)

include(CTest)

add_executable(raid0_ut
    raid0.cpp
)

target_link_libraries(raid0_ut PRIVATE
    gtest::gtest
    ublk::raid0
)

add_test(NAME RAID0 COMMAND raid0_ut)

add_executable(raid1_ut
    raid1.cpp
)

target_link_libraries(raid1_ut PRIVATE
    gtest::gtest
    ublk::raid1
)

add_test(NAME RAID1 COMMAND raid1_ut)
