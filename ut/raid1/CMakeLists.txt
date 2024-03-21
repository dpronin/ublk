add_executable(raid1_ut
    raid1.cpp
)

target_link_libraries(raid1_ut PRIVATE
    gtest::gtest
    ublk::mm
    ublk::raid1
    ublk::ut::helpers
    ublk::utils
)

add_test(NAME RAID1 COMMAND raid1_ut)
