find_package(libnl REQUIRED)
find_package(readline REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(Boost REQUIRED COMPONENT headers)

add_executable(${PROJECT_NAME}
    bdev_create_param.hpp
    bdev_destroy_param.hpp
    cfq_base.hpp
    cfq_popper.hpp
    cfq_pos.hpp
    cfq_pusher.hpp
    cli_ctx.hpp
    cli_state.hpp
    cmd.hpp
    cmd_acknowledger.cpp
    cmd_acknowledger.hpp
    cmd_args.hpp
    cmd_bdev_create.hpp
    cmd_bdev_destroy.hpp
    cmd_flush_handler.hpp
    cmd_flush_handler_adaptor.hpp
    cmd_handler.cpp
    cmd_handler.hpp
    cmd_handler_factory.hpp
    cmd_handler_interface.hpp
    cmd_handler_not_supp.hpp
    cmd_help.hpp
    cmd_interface.hpp
    cmd_invoker.hpp
    cmd_op_err_handler.hpp
    cmd_parser_default.cpp
    cmd_parser_default.hpp
    cmd_parser_interface.hpp
    cmd_quit.hpp
    cmd_read_handler.cpp
    cmd_read_handler.hpp
    cmd_read_handler_adaptor.hpp
    cmd_state.hpp
    cmd_write_handler.cpp
    cmd_write_handler.hpp
    cmd_write_handler_adaptor.hpp
    color.hpp
    dummy_flush_handler.hpp
    dummy_read_handler.hpp
    dummy_write_handler.hpp
    flush_handler.hpp
    flush_handler_interface.hpp
    genl.hpp
    handler.cpp
    handler.hpp
    main.cpp
    main_state.cpp
    main_state.hpp
    master.cpp
    master.hpp
    qublkcmd.hpp
    read_handler.hpp
    read_handler_interface.hpp
    readline.cpp
    readline.hpp
    slave.cpp
    slave.hpp
    suggestions_lister_interface.hpp
    sysfs_attr.hpp
    types.hpp
    uread.hpp
    utility.hpp
    write_handler.hpp
    write_handler_interface.hpp
)

if (NOT ${CMAKE_CXX_COMPILER} MATCHES ".*clang.*")
    target_compile_options(${PROJECT_NAME} PRIVATE -Wno-interference-size)
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    fmt::fmt
    spdlog::spdlog
    libnl::libnl
    readline::readline
    Boost::headers
    headers
)
