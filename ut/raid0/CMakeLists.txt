add_executable(raid0_ut
    backend_failure.cpp
    base.hpp
    chunk_by_chunk.cpp
    go_to_offline_due_to_backend_failure.cpp
    stripe_by_stripe.cpp
)

target_link_libraries(raid0_ut PRIVATE
    ublk::raid0
    ublk::ut
)

add_test(NAME RAID0 COMMAND raid0_ut)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(raid0_ut)
  setup_target_for_coverage_gcovr_html(NAME raid0_ut_coverage EXECUTABLE raid0_ut)
endif ()
