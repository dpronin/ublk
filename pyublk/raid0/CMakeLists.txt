add_library(ublk_raid0 STATIC
    backend.cpp
    backend.hpp
    fsm.hpp
    rdq_submitter.hpp
    target.cpp
    target.hpp
    wrq_submitter.hpp
)

target_include_directories(ublk_raid0 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_raid0 PUBLIC
    Boost::system
    ublk::mm
    ublk::utils
)
target_link_libraries(ublk_raid0 PRIVATE
    sml::sml
)
set_property(TARGET ublk_raid0 PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(ublk::raid0 ALIAS ublk_raid0)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(ublk_raid0)
endif ()
