add_executable(raid5_ut
    raid5.cpp
)

target_link_libraries(raid5_ut PRIVATE
    gtest::gtest
    ublk::mm
    ublk::raid5
    ublk::ut::helpers
    ublk::utils
)

add_test(NAME RAID5 COMMAND raid5_ut)
