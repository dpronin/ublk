add_library(ublk_null INTERFACE
    discard_handler.hpp
    flq_submitter.hpp
    rdq_submitter.hpp
    wrq_submitter.hpp
)

target_include_directories(ublk_null INTERFACE ${CMAKE_CURRENT_LIST_DIR}/..)
# target_link_libraries(ublk_null PRIVATE)

add_library(ublk::null ALIAS ublk_null)
