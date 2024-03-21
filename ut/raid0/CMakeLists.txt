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

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(raid0_ut)
  setup_target_for_coverage_gcovr_html(NAME raid0_ut_coverage EXECUTABLE raid0_ut)
endif ()
