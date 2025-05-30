find_package(spdlog CONFIG REQUIRED)
find_package(sml CONFIG REQUIRED)
find_package(liburing CONFIG REQUIRED)

add_library(${PROJECT_NAME} SHARED
    cells_holder.hpp
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
    factory_interface.hpp
    factory_shared_interface.hpp
    factory_unique_interface.hpp
    flq_submitter_composite.hpp
    flq_submitter_interface.hpp
    for_each_celld.hpp
    handler.cpp
    handler.hpp
    handler_interface.hpp
    master.cpp
    master.hpp
    qublkcmd.hpp
    rdq_submitter.hpp
    rdq_submitter_interface.hpp
    req_handler_not_supp.hpp
    req_op_err_handler.hpp
    rw_handler.hpp
    rw_handler_interface.hpp
    sector.hpp
    slave.cpp
    slave.hpp
    slave_param.hpp
    submitter_interface.hpp
    target.hpp
    target_properties.hpp
    discard_query.hpp
    discard_req.hpp
    flush_query.hpp
    flush_req.hpp
    query.hpp
    read_query.hpp
    read_req.hpp
    req.hpp
    ublk_req_handler_interface.hpp
    write_query.hpp
    write_req.hpp
    wrq_submitter_interface.hpp
)
set_target_properties(${PROJECT_NAME} PROPERTIES VERSION ${PROJECT_VERSION} SOVERSION ${PROJECT_VERSION_MAJOR})

add_subdirectory(cfq)
add_subdirectory(cache)
add_subdirectory(def)
add_subdirectory(inmem)
add_subdirectory(null)
add_subdirectory(raid0)
add_subdirectory(raid1)
add_subdirectory(raid4)
add_subdirectory(raid5)
add_subdirectory(raidsp)

target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR})
target_include_directories(${PROJECT_NAME} PUBLIC ${CMAKE_CURRENT_LIST_DIR}/..)

target_link_libraries(${PROJECT_NAME} PRIVATE
    Boost::headers
    Boost::system
    spdlog::spdlog
    Microsoft.GSL::GSL
    ublk::cache
    ublk::cfq
    ublk::def
    ublk::inmem
    ublk::mm
    ublk::null
    ublk::raid0
    ublk::raid1
    ublk::raid4
    ublk::raid5
    ublk::sys
    ublk::utils
)

if (NOT ${CMAKE_BUILD_TYPE} MATCHES "Debug|RelWithDebInfo")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_STRIP} $<TARGET_FILE:${PROJECT_NAME}>)
endif()

install(TARGETS ${PROJECT_NAME} LIBRARY)

add_library(ublk::ublk ALIAS ${PROJECT_NAME})

