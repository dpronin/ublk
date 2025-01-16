add_library(ublk_inmem STATIC
    rdq_submitter.hpp
    target.cpp
    target.hpp
    wrq_submitter.hpp
)

target_include_directories(ublk_inmem PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_inmem PUBLIC
    Boost::system
    Microsoft.GSL::GSL
    ublk::utils
    ublk::mm
)

set_target_properties(ublk_inmem PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(ublk::inmem ALIAS ublk_inmem)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(ublk_inmem)
endif ()
