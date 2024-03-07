find_package(benchmark CONFIG REQUIRED)

if (COVERAGE_ENABLE)
  include(CodeCoverage)
endif ()

foreach(bench fill copy xor)
    set(EXECUTABLE_NAME ${PROJECT_NAME}_${bench}_bench)
    add_executable(${EXECUTABLE_NAME}
        ${bench}.cpp
    )

    target_link_libraries(${EXECUTABLE_NAME} PRIVATE
        benchmark::benchmark
        ublk::utils
        ublk::mm
    )
endforeach()
