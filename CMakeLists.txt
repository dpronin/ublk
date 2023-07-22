cmake_minimum_required(VERSION 3.19 FATAL_ERROR)
project(ublk VERSION 2.3.0 LANGUAGES CXX)

if (NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXPORT_COMPILE_COMMANDS true)
set(CMAKE_CXX_STANDARD_REQUIRED on)

find_package(Boost REQUIRED COMPONENT headers)

add_library(headers INTERFACE
    include/align.hpp
    include/concepts.hpp
    include/factory_interface.hpp
    include/factory_shared_interface.hpp
    include/factory_unique_interface.hpp
    include/file.hpp
    include/handler_interface.hpp
    include/mapping.hpp
    include/mem.hpp
    include/rvwrap.hpp
    include/uread.hpp
    include/utility.hpp
)

target_include_directories(headers INTERFACE include)
target_link_libraries(headers INTERFACE
    Boost::headers
)

add_subdirectory(src)
