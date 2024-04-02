add_library(ublk_cfq INTERFACE
    base.hpp
    popper.hpp
    pos.hpp
    pusher.hpp
)

target_include_directories(ublk_cfq INTERFACE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_cfq INTERFACE
    ublk::utils
)

add_library(ublk::cfq ALIAS ublk_cfq)
