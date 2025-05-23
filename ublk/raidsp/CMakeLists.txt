add_library(ublk_raidsp STATIC
    acceptor.cpp
    acceptor.hpp
    backend.cpp
    backend.hpp
    fsm.hpp
    parity.cpp
    parity.hpp
    target.cpp
    target.hpp
)

target_include_directories(ublk_raidsp PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_raidsp PUBLIC
    Boost::system
    ublk::utils
    ublk::mm
)
target_link_libraries(ublk_raidsp PRIVATE
    Microsoft.GSL::GSL
    sml::sml
)

set_target_properties(ublk_raidsp PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(ublk::raidsp ALIAS ublk_raidsp)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(ublk_raidsp)
endif ()
