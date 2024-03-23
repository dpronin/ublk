add_executable(raid4_ut
    go_to_offline_due_to_backend_failure.cpp
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

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(raid4_ut)
  setup_target_for_coverage_gcovr_html(NAME raid4_ut_coverage EXECUTABLE raid4_ut)
endif ()
