file(READ ublksh.py.in UBLKSH_PY_CODE)

configure_file(ublksh.py.cpp.in ${CMAKE_BINARY_DIR}/ublksh.py.cpp @ONLY)
configure_file(ublksh.py.hpp.in ${CMAKE_BINARY_DIR}/ublksh.py.hpp @ONLY)

add_executable(ublksh
    ublksh.cpp
    ${CMAKE_BINARY_DIR}/ublksh.py.cpp
    ${CMAKE_BINARY_DIR}/ublksh.py.hpp
)
target_compile_options(ublksh PRIVATE
    -include${CMAKE_BINARY_DIR}/ublksh.py.hpp
)

target_link_libraries(ublksh PRIVATE
    ublk::ublk
)

target_link_libraries(ublksh PRIVATE
    pybind11::embed
    $<$<CONFIG:Release>:pybind11::lto>
)

if (NOT ${CMAKE_BUILD_TYPE} MATCHES "Debug|RelWithDebInfo")
    pybind11_strip(ublksh)
endif()

install(TARGETS ublksh RUNTIME)
