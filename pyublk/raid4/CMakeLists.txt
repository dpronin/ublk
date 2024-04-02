add_library(ublk_raid4 STATIC
    rdq_submitter.hpp
    target.cpp
    target.hpp
    wrq_submitter.hpp
)

target_include_directories(ublk_raid4 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_raid4 PUBLIC
    Boost::system
    ublk::mm
    ublk::raidsp
    ublk::utils
)
set_property(TARGET ublk_raid4 PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(ublk::raid4 ALIAS ublk_raid4)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(ublk_raid4)
endif ()
