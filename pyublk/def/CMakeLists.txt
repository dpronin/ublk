add_library(ublk_def STATIC
    flush_handler.hpp
    read_handler.hpp
    target.cpp
    target.hpp
    write_handler.hpp
)

target_include_directories(ublk_def PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_def PUBLIC
    ublk::utils
    ublk::mm
    liburing::liburing
)
target_link_libraries(ublk_def PUBLIC
    Boost::system
)
set_property(TARGET ublk_def PROPERTY POSITION_INDEPENDENT_CODE ON)

add_library(ublk::def ALIAS ublk_def)

if (NOT CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(ublk_def)
endif ()
