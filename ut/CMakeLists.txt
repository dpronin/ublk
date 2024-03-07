find_package(GTest CONFIG REQUIRED)

include(CTest)

add_executable(raid0_ut
    raid0.cpp
)

target_link_libraries(raid0_ut PRIVATE
    gtest::gtest
    ublk::raid0
    ublk::utils
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

add_executable(raid4_ut
    raid4.cpp
)

target_link_libraries(raid4_ut PRIVATE
    gtest::gtest
    ublk::raid4
)

add_test(NAME RAID4 COMMAND raid4_ut)

add_executable(raid5_ut
    raid5.cpp
)

target_link_libraries(raid5_ut PRIVATE
    gtest::gtest
    ublk::raid5
)

add_test(NAME RAID5 COMMAND raid5_ut)
