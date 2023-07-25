find_package(libnl REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(Boost REQUIRED COMPONENT headers)
find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 REQUIRED)

pybind11_add_module(${PROJECT_NAME}
    align.hpp
    aligned_allocator.hpp
    cache_line_aligned_allocator.hpp
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
    concepts.hpp
    creator_interface.hpp
    destroyer_interface.hpp
    discard_handler_interface.hpp
    dummy_discard_handler.hpp
    dummy_flush_handler.hpp
    dummy_read_handler.hpp
    dummy_write_handler.hpp
    epoll.hpp
    factory_interface.hpp
    factory_shared_interface.hpp
    factory_unique_interface.hpp
    file.hpp
    flush_handler.hpp
    flush_handler_interface.hpp
    for_each_celld.hpp
    genl.hpp
    handler.cpp
    handler.hpp
    handler_interface.hpp
    mapper_interface.hpp
    mapping.hpp
    master.cpp
    master.hpp
    mem.hpp
    page.hpp
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
    slave.cpp
    slave.hpp
    slave_param.hpp
    target.hpp
    target_properties.hpp
    ublk_req.hpp
    ublk_req_handler_interface.hpp
    unmapper_interface.hpp
    uread.hpp
    utility.hpp
    write_handler.hpp
    write_handler_interface.hpp
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    fmt::fmt
    spdlog::spdlog
    libnl::libnl
    Boost::headers
)

target_compile_features(${PROJECT_NAME} PRIVATE
    cxx_std_23
)
