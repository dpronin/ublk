find_package(libnl REQUIRED)
find_package(readline REQUIRED)
find_package(fmt REQUIRED)
find_package(spdlog REQUIRED)
find_package(Boost REQUIRED COMPONENT headers)

add_executable(${PROJECT_NAME}
    bdev_mapper.hpp
    bdev_unmapper.hpp
    cfq_base.hpp
    cfq_popper.hpp
    cfq_pos.hpp
    cfq_pusher.hpp
    cli/bdev_map_param.hpp
    cli/bdev_mapper_interface.hpp
    cli/bdev_unmap_param.hpp
    cli/bdev_unmapper_interface.hpp
    cli/cli_ctx.hpp
    cli/cli_state.hpp
    cli/cmd.hpp
    cli/cmd_args.hpp
    cli/cmd_bdev_map.hpp
    cli/cmd_bdev_unmap.hpp
    cli/cmd_help.hpp
    cli/cmd_interface.hpp
    cli/cmd_invoker.hpp
    cli/cmd_parser_default.cpp
    cli/cmd_parser_default.hpp
    cli/cmd_parser_interface.hpp
    cli/cmd_quit.hpp
    cli/cmd_state.hpp
    cli/cmd_target_create.hpp
    cli/cmd_target_destroy.hpp
    cli/color.hpp
    cli/main_state.cpp
    cli/main_state.hpp
    cli/readline.cpp
    cli/readline.hpp
    cli/suggestions_lister_interface.hpp
    cli/target_create_param.hpp
    cli/target_creator_interface.hpp
    cli/target_destroy_param.hpp
    cli/target_destroyer_interface.hpp
    cli/types.hpp
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
    dummy_discard_handler.hpp
    dummy_flush_handler.hpp
    dummy_read_handler.hpp
    dummy_write_handler.hpp
    flush_handler.hpp
    flush_handler_interface.hpp
    genl.hpp
    handler.cpp
    handler.hpp
    main.cpp
    mapper_interface.hpp
    master.cpp
    master.hpp
    qublkcmd.hpp
    read_handler.hpp
    read_handler_interface.hpp
    req_handler.cpp
    req_handler.hpp
    req_handler_not_supp.hpp
    req_op_err_handler.hpp
    slave.cpp
    slave.hpp
    slave_param.hpp
    target.hpp
    target_creator.hpp
    target_destroyer.hpp
    target_properties.hpp
    ublk_req.hpp
    ublk_req_handler_interface.hpp
    unmapper_interface.hpp
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
