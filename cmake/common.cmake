add_compile_options(-Wall -Wextra -march=native)

if (${CMAKE_BUILD_TYPE} MATCHES "Release" OR ${CMAKE_BUILD_TYPE} MATCHES "RelWithDebInfo")
    set(CMAKE_INTERPROCEDURAL_OPTIMIZATION TRUE)
endif()
