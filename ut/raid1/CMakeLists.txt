add_executable(raid1_ut
    backend_failure.cpp
    go_to_offline_due_to_backend_failure.cpp
    raid1.cpp
)

target_link_libraries(raid1_ut PRIVATE
    ublk::raid1
    ublk::ut
)

add_test(NAME RAID1 COMMAND raid1_ut)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(raid1_ut)
  setup_target_for_coverage_gcovr_html(NAME raid1_ut_coverage EXECUTABLE raid1_ut)
endif ()
