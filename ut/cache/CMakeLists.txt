add_executable(cache_ut
    flat_lru.cpp
)

target_link_libraries(cache_ut PRIVATE
    gtest::gtest
    ublk::mm
    ublk::cache
    ublk::ut::helpers
    ublk::utils
)

add_test(NAME CACHE COMMAND cache_ut)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(cache_ut)
  setup_target_for_coverage_gcovr_html(NAME cache_ut_coverage EXECUTABLE cache_ut)
endif ()
