cmake_minimum_required(VERSION 3.12)

project(ublk
        VERSION 2.5.31
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

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_compile_options(
  -march=native
  -Wall
  -Wextra
  -Werror
  -DBOOST_ASIO_HAS_IO_URING=1
  $<$<NOT:$<STREQUAL:$<LOWER_CASE:${CMAKE_CXX_COMPILER_ID}>,clang>>:-Wno-interference-size>
  $<$<CONFIG:Debug>:-ggdb3>
)

if (${CMAKE_BUILD_TYPE} MATCHES "Release" OR ${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo")
  include(CheckIPOSupported)
  check_ipo_supported(RESULT result OUTPUT output)
  if (result)
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
  else ()
    message(WARNING "IPO is not supported: ${output}")
  endif ()
  set(CMAKE_C_VISIBILITY_PRESET hidden)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
  set(CMAKE_VISIBILITY_INLINES_HIDDEN TRUE)
endif()

configure_file(config.hpp.in ${CMAKE_BINARY_DIR}/config.hpp @ONLY)
add_compile_options(-include ${CMAKE_BINARY_DIR}/config.hpp)

find_package(spdlog CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENT headers system)
find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG REQUIRED)
find_package(eve CONFIG REQUIRED)
find_package(sml CONFIG REQUIRED)
find_package(liburing CONFIG REQUIRED)
find_package(libnl CONFIG REQUIRED)

add_subdirectory(sys)
add_subdirectory(utils)
add_subdirectory(mm)
add_subdirectory(pyublk)

if (UT_ENABLE)
  add_subdirectory(ut)
endif ()

if (BENCH_ENABLE)
  add_subdirectory(bench)
endif ()
