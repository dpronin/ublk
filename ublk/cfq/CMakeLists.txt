add_library(ublk_cfq INTERFACE
    base.hpp
    popper.hpp
    pos.hpp
    pusher.hpp
)

target_include_directories(ublk_cfq INTERFACE ${CMAKE_CURRENT_LIST_DIR}/..)
target_link_libraries(ublk_cfq INTERFACE
    Microsoft.GSL::GSL
    ublk::utils
)
set_target_properties(ublk_cfq PROPERTIES POSITION_INDEPENDENT_CODE ON)

add_library(ublk::cfq ALIAS ublk_cfq)
