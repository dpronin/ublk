find_package(libnl REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(Boost CONFIG REQUIRED COMPONENT headers system)
find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG REQUIRED)
find_package(eve CONFIG REQUIRED)

add_library(utils INTERFACE
    algo.hpp
    concepts.hpp
    file.hpp
    math.hpp
    mem.hpp
    mem_types.hpp
    page.hpp
    span.hpp
    utility.hpp
)

target_include_directories(utils INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(utils INTERFACE
    eve::eve
    Boost::system
)

pybind11_add_module(${PROJECT_NAME}
    align.hpp
    aligned_allocator.hpp
    cache_line_aligned_allocator.hpp
    cached_rw_handler.cpp
    cached_rw_handler.hpp
    cfq_base.hpp
    cfq_popper.hpp
    cfq_pos.hpp
    cfq_pusher.hpp
    cmd_acknowledger.cpp
    cmd_acknowledger.hpp
    cmd_discard_handler.hpp
    cmd_discard_handler_adaptor.hpp
    cmd_flush_handler.hpp
    cmd_flush_handler_adaptor.hpp
    cmd_handler.cpp
    cmd_handler.hpp
    cmd_handler_factory.hpp
    cmd_read_handler.cpp
    cmd_read_handler.hpp
    cmd_read_handler_adaptor.hpp
    cmd_write_handler.cpp
    cmd_write_handler.hpp
    cmd_write_handler_adaptor.hpp
    discard_handler_interface.hpp
    epoll.hpp
    factory_interface.hpp
    factory_shared_interface.hpp
    factory_unique_interface.hpp
    flat_lru_cache.hpp
    flush_handler_composite.hpp
    flush_handler_interface.hpp
    for_each_celld.hpp
    genl.hpp
    handler.cpp
    handler.hpp
    handler_interface.hpp
    master.cpp
    master.hpp
    page_aligned_allocator.hpp
    pyublk.cpp
    qublkcmd.hpp
    read_handler.hpp
    read_handler_interface.hpp
    req_handler.cpp
    req_handler.hpp
    req_handler_not_supp.hpp
    req_op_err_handler.hpp
    rvwrap.hpp
    rw_handler.hpp
    rw_handler_interface.hpp
    sector.hpp
    slave.cpp
    slave.hpp
    slave_param.hpp
    target.hpp
    target_properties.hpp
    ublk_req.hpp
    ublk_req_handler_interface.hpp
    write_handler_interface.hpp
)

add_subdirectory(def)
add_subdirectory(inmem)
add_subdirectory(null)
add_subdirectory(raid0)
add_subdirectory(raid1)
add_subdirectory(raid4)
add_subdirectory(raid5)

target_include_directories(${PROJECT_NAME} PRIVATE ${CMAKE_CURRENT_LIST_DIR})

target_link_libraries(${PROJECT_NAME} PRIVATE
    utils
    spdlog::spdlog
    libnl::libnl
    Boost::headers
    Boost::system
    ublk::def
    ublk::inmem
    ublk::null
    ublk::raid0
    ublk::raid1
    ublk::raid4
    ublk::raid5
)
