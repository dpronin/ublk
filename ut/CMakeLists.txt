find_package(GTest CONFIG REQUIRED)

add_executable(${PROJECT_NAME}_ut
    main.cpp
)

target_link_libraries(${PROJECT_NAME}_ut PRIVATE
    gtest::gtest
)
