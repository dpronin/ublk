add_library(ublk_raid1 STATIC
    read_handler.hpp
    target.cpp
    target.hpp
    write_handler.hpp
)

target_include_directories(ublk_raid1 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_raid1 PUBLIC
    Boost::system
    sml::sml
    ublk::mm
    ublk::utils
)
set_property(TARGET ublk_raid1 PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(ublk::raid1 ALIAS ublk_raid1)

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(ublk_raid1)
endif ()
