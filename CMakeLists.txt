cmake_minimum_required(VERSION 3.12)

project(ublk
        VERSION 5.1.1
        HOMEPAGE_URL "https://github.com/dpronin/ublk"
        LANGUAGES CXX
        DESCRIPTION "Linux userspace python module that supports negotiating with ublkdrv and receiving block IO from the driver by using shared memory within UIO")

option(UT_ENABLE "enable unit tests to compile along")
option(BENCH_ENABLE "enable benchmarks to compile along")
option(COVERAGE_ENABLE "enable coverage to compile along")

if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

list(APPEND CMAKE_MODULE_PATH
    ${CMAKE_CURRENT_SOURCE_DIR}/cmake
)

include(LtoConfigure)
include(VisibilityConfigure)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib")

add_compile_options(
  -march=native
  -Wall
  -Wextra
  -Werror
  $<$<NOT:$<STREQUAL:$<LOWER_CASE:${CMAKE_CXX_COMPILER_ID}>,clang>>:-Wno-interference-size>
  $<$<CONFIG:Debug>:-ggdb3>
  $<$<CONFIG:Release>:-fno-stack-protector>
)

lto_setup()
visibility_setup()

configure_file(config.hpp.in ${CMAKE_BINARY_DIR}/config.hpp @ONLY)
add_compile_options(-include ${CMAKE_BINARY_DIR}/config.hpp)

find_package(Boost CONFIG REQUIRED COMPONENT headers system)
find_package(Microsoft.GSL CONFIG REQUIRED)

add_subdirectory(sys)
add_subdirectory(utils)
add_subdirectory(mm)
add_subdirectory(ublk)
add_subdirectory(ublksh)

if (UT_ENABLE)
  add_subdirectory(ut)
endif ()

if (BENCH_ENABLE)
  add_subdirectory(bench)
endif ()
