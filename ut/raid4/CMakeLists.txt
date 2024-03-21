add_executable(raid4_ut
    raid4.cpp
)

target_link_libraries(raid4_ut PRIVATE
    gtest::gtest
    ublk::mm
    ublk::raid4
    ublk::ut::helpers
    ublk::utils
)

add_test(NAME RAID4 COMMAND raid4_ut)
