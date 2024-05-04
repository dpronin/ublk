find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG REQUIRED)

include(EmbedResources)

add_embedded_text_resources(
  ublk_resource_ublksh_py_code
  OUT_DIR resources
  HEADER ublksh.py.hpp
  NAMESPACE ublk::resources
  RESOURCE_NAMES ublksh_py_code
  RESOURCES
    ${CMAKE_CURRENT_LIST_DIR}/resources/ublksh.py.in
)
add_library(ublk::resourses::ublksh_py_code ALIAS ublk_resource_ublksh_py_code)

add_executable(ublksh
    ublksh.cpp
)

target_link_libraries(ublksh PRIVATE
    ublk::ublk
    ublk::resourses::ublksh_py_code
)

target_link_libraries(ublksh PRIVATE
    pybind11::embed
    $<$<CONFIG:Release>:pybind11::lto>
)

if (NOT ${CMAKE_BUILD_TYPE} MATCHES "Debug|RelWithDebInfo")
    pybind11_strip(ublksh)
endif()

install(TARGETS ublksh RUNTIME)
