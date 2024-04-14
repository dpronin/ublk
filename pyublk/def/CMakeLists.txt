add_library(ublk_def STATIC
    flq_submitter.hpp
    rdq_submitter.hpp
    target.cpp
    target.hpp
    wrq_submitter.hpp
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

target_compile_definitions(ublk_def PUBLIC
    BOOST_ASIO_HAS_IO_URING=1
)

add_library(ublk::def ALIAS ublk_def)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(ublk_def)
endif ()
