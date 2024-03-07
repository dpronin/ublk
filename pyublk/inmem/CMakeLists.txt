add_library(ublk_inmem STATIC
    read_handler.hpp
    target.cpp
    target.hpp
    write_handler.hpp
)

target_include_directories(ublk_inmem PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_inmem PUBLIC
    Boost::system
    ublk::utils
    ublk::mm
)
set_property(TARGET ublk_inmem PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(ublk::inmem ALIAS ublk_inmem)

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(ublk_inmem)
endif ()
