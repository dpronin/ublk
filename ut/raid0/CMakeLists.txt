add_executable(raid0_ut
    raid0.cpp
)

target_link_libraries(raid0_ut PRIVATE
    gtest::gtest
    ublk::mm
    ublk::raid0
    ublk::ut::helpers
    ublk::utils
)

add_test(NAME RAID0 COMMAND raid0_ut)
