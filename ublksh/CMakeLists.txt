add_executable(ublksh
    ublksh.cpp
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
