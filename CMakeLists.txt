cmake_minimum_required(VERSION 3.19)

project(ublk
        VERSION 5.0.0
        HOMEPAGE_URL "https://github.com/dpronin/ublk"
        LANGUAGES CXX
        DESCRIPTION "Linux userspace python module that supports negotiating with ublkdrv and receiving block IO from the driver by using shared memory within UIO")

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

add_compile_options(
  -march=native
  -Wall
  -Wextra
  -Werror
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

add_subdirectory(pyublk)
