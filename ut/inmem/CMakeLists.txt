add_executable(inmem_ut
    inmem.cpp
)

target_link_libraries(inmem_ut PRIVATE
    ublk::inmem
    ublk::ut
)

add_test(NAME INMEM COMMAND inmem_ut)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(inmem_ut)
  setup_target_for_coverage_gcovr_html(NAME inmem_ut_coverage EXECUTABLE inmem_ut)
endif ()
