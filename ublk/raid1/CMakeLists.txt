add_library(ublk_raid1 STATIC
    backend.cpp
    backend.hpp
    fsm.hpp
    rdq_submitter.hpp
    target.cpp
    target.hpp
    wrq_submitter.hpp
)

target_include_directories(ublk_raid1 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_raid1 PUBLIC
    Boost::system
    ublk::mm
    ublk::utils
)
target_link_libraries(ublk_raid1 PRIVATE
    sml::sml
)

set_target_properties(ublk_raid1 PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(ublk::raid1 ALIAS ublk_raid1)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(ublk_raid1)
endif ()
