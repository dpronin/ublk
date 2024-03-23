add_executable(raid5_ut
    go_to_offline_due_to_backend_failure.cpp
    raid5.cpp
    stripe_parity.cpp
)

target_link_libraries(raid5_ut PRIVATE
    gtest::gtest
    ublk::mm
    ublk::raid5
    ublk::ut::helpers
    ublk::utils
)

add_test(NAME RAID5 COMMAND raid5_ut)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(raid5_ut)
  setup_target_for_coverage_gcovr_html(NAME raid5_ut_coverage EXECUTABLE raid5_ut)
endif ()
