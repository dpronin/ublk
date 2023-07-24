cmake_minimum_required(VERSION 3.19)

project(ublk
        VERSION 3.1.5
        HOMEPAGE_URL "https://github.com/dpronin/ublk"
        LANGUAGES CXX
        DESCRIPTION "Linux userspace application that supports negotiating with ublkdrv and receiving block IO from the driver by using shared memory within UIO")

include(CTest)

option(BUILD_RPM "Create RPM from the project. rpmbuild is required")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

include(common)
include(params)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED on)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)

find_package(Boost REQUIRED COMPONENT headers)

add_library(headers INTERFACE
    include/align.hpp
    include/aligned_allocator.hpp
    include/cache_line_aligned_allocator.hpp
    include/concepts.hpp
    include/epoll.hpp
    include/factory_interface.hpp
    include/factory_shared_interface.hpp
    include/factory_unique_interface.hpp
    include/file.hpp
    include/handler_interface.hpp
    include/mapping.hpp
    include/mem.hpp
    include/page.hpp
    include/page_aligned_allocator.hpp
    include/rvwrap.hpp
    include/uread.hpp
    include/utility.hpp
)

target_include_directories(headers INTERFACE include)
target_link_libraries(headers INTERFACE
    Boost::headers
)

if (BUILD_TESTING)
    find_package(GTest)
    if (GTest_FOUND)
        if (COVERAGE)
            include(code_coverage)
            append_coverage_compiler_flags()
        endif()

        if (SANITIZER)
            set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=undefined")
            if (${SANITIZER} MATCHES "ASAN")
                set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=address")
            elseif(${SANITIZER} MATCHES "TSAN")
                set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=thread")
            endif()
            if (${CMAKE_CXX_COMPILER} MATCHES ".*clang.*")
                set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fsanitize=implicit-conversion")
            endif()
        endif ()
    endif ()
endif ()

add_subdirectory(etc)
add_subdirectory(src)

if (BUILD_RPM)
    add_subdirectory(rpm)
endif (BUILD_RPM)
