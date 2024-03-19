add_library(ublk_raid5 STATIC
    read_handler.hpp
    target.cpp
    target.hpp
    write_handler.hpp
)

target_include_directories(ublk_raid5 PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_raid5 PUBLIC
    Boost::system
    ublk::mm
    ublk::raidsp
    ublk::utils
)
target_link_libraries(ublk_raid5 PRIVATE
    sml::sml
)
set_property(TARGET ublk_raid5 PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(ublk::raid5 ALIAS ublk_raid5)

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(ublk_raid5)
endif ()
