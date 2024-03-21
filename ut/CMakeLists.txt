find_package(GTest CONFIG REQUIRED)

include(CTest)

add_library(ublk_ut_helpers INTERFACE
    helpers.hpp
)
target_link_libraries(ublk_ut_helpers INTERFACE
    gtest::gtest
)
target_include_directories(ublk_ut_helpers INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

add_library(ublk::ut::helpers ALIAS ublk_ut_helpers)

add_subdirectory(raid0)
add_subdirectory(raid1)
add_subdirectory(raid4)
add_subdirectory(raid5)

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(raid0_ut)
  setup_target_for_coverage_gcovr_html(NAME raid0_ut_coverage EXECUTABLE raid0_ut)

  append_coverage_compiler_flags_to_target(raid1_ut)
  setup_target_for_coverage_gcovr_html(NAME raid1_ut_coverage EXECUTABLE raid1_ut)

  append_coverage_compiler_flags_to_target(raid4_ut)
  setup_target_for_coverage_gcovr_html(NAME raid4_ut_coverage EXECUTABLE raid4_ut)

  append_coverage_compiler_flags_to_target(raid5_ut)
  setup_target_for_coverage_gcovr_html(NAME raid5_ut_coverage EXECUTABLE raid5_ut)
endif ()
