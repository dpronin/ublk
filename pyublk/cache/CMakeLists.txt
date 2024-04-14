add_library(ublk_cache STATIC
    flat_lru.hpp
    rw_handler.cpp
    rw_handler.hpp
    rwi_handler.cpp
    rwi_handler.hpp
    rwt_handler.cpp
    rwt_handler.hpp
)

target_include_directories(ublk_cache PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_cache PRIVATE
    Boost::headers
    Boost::system
    ublk::utils
    ublk::mm
)

add_library(ublk::cache ALIAS ublk_cache)

if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND COVERAGE_ENABLE)
  include(CodeCoverage)
  append_coverage_compiler_flags_to_target(ublk_cache)
endif ()
