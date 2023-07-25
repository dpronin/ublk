#include "master.hpp"

#include <cstring>

#include <fcntl.h>
#include <future>
#include <netlink/attr.h>
#include <sys/wait.h>
#include <unistd.h>

#include <linux/ublk/genl.h>

#include <memory>
#include <stdexcept>
#include <utility>

#include <fmt/format.h>
#include <spdlog/spdlog.h>

#include "file.hpp"
#include "genl.hpp"
#include "req_handler.hpp"
#include "slave.hpp"

#include "cmd_discard_handler.hpp"
#include "cmd_discard_handler_adaptor.hpp"
#include "cmd_flush_handler.hpp"
#include "cmd_flush_handler_adaptor.hpp"
#include "cmd_read_handler.hpp"
#include "cmd_read_handler_adaptor.hpp"
#include "cmd_write_handler.hpp"
#include "cmd_write_handler_adaptor.hpp"
#include "discard_handler_interface.hpp"
#include "dummy_discard_handler.hpp"
#include "dummy_flush_handler.hpp"
#include "dummy_read_handler.hpp"
#include "dummy_write_handler.hpp"
#include "flush_handler.hpp"
#include "flush_handler_interface.hpp"
#include "read_handler.hpp"
#include "read_handler_interface.hpp"
#include "target.hpp"
#include "ublk_req_handler_interface.hpp"
#include "write_handler.hpp"
#include "write_handler_interface.hpp"

using namespace ublk;

Master::~Master() {
  /* While there are children_ we would wait for them all to exit */
  while (!children_.empty()) {
    [[maybe_unused]] auto &[child_pid, _] = *children_.begin();
    if (kill(child_pid, SIGTERM) < 0)
      kill(child_pid, SIGKILL);
    children_.erase(child_pid);
  }
}

void Master::map(bdev_map_param const &param) {
  auto target = std::shared_ptr<Target>{};

  if (auto it = targets_.find(param.target_name); targets_.end() != it)
    target = it->second;

  if (!target)
    throw std::invalid_argument(
        fmt::format("'{}' target does not exist", param.target_name));

  auto nl_sock = genl::sock_open();
  auto msg = genl::msg_alloc();

  genlmsg_put(msg.get(), 0, 0, genl::resolve(*nl_sock), 0, 0,
              UBLK_GENL_BDEV_CMD_CREATE, 1);

  nla_put_string(msg.get(), UBLK_GENL_BDEV_ATTR_NAME_SUFFIX,
                 param.bdev_suffix.c_str());
  nla_put_u64(msg.get(), UBLK_GENL_BDEV_ATTR_CAPACITY_SECTORS,
              target->properties().capacity_sectors);

  if (param.read_only)
    nla_put_flag(msg.get(), UBLK_GENL_BDEV_ATTR_READ_ONLY);

  genl::auto_send(*nl_sock, *msg);

  nl_sock.reset();

  if (auto const child_pid = fork(); 0 == child_pid) {
    spdlog::set_pattern("[slave %P] [%^%l%$]: %v");
    spdlog::info("started: {}", param.target_name);
    slave::run({
        .bdev_suffix = param.bdev_suffix,
        .handler = target->req_handler(),
    });
  } else if (child_pid > 0) {
    /* We're in a parent's body, remember a new child's PID */
    children_.emplace(
        child_pid, std::async(std::launch::async, [child_pid, target] mutable {
          int wstatus{0};
          auto const childw = waitpid(child_pid, &wstatus, 0);
          if (WIFEXITED(wstatus))
            spdlog::info("childw {} (expected {}) exited", childw, child_pid);
          target.reset();
        }));
  } else {
    throw std::runtime_error(fmt::format(
        "{}: failed to create child, reason: {}", getpid(), strerror(errno)));
  }
}

void Master::unmap(bdev_unmap_param const &param) {
  auto nl_sock = genl::sock_open();
  auto msg = genl::msg_alloc();

  genlmsg_put(msg.get(), 0, 0, genl::resolve(*nl_sock), 0, 0,
              UBLK_GENL_BDEV_CMD_DESTROY, 1);
  nla_put_string(msg.get(), UBLK_GENL_BDEV_ATTR_NAME_SUFFIX,
                 param.bdev_suffix.data());

  genl::auto_send(*nl_sock, *msg);
}

void Master::create(target_create_param const &param) {
  if (targets_.contains(param.name))
    throw std::invalid_argument(
        fmt::format("{} target already exists", param.name));

  auto reader = std::shared_ptr<IReadHandler>{};
  auto writer = std::shared_ptr<IWriteHandler>{};
  auto flusher = std::shared_ptr<IFlushHandler>{};
  auto discarder = std::shared_ptr<IDiscardHandler>{};

  if (param.path != "dummy") {
    auto fd_target = std::shared_ptr{open(
        param.path, O_RDWR | O_CREAT, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH)};
    if (!fd_target)
      throw std::runtime_error(
          fmt::format("could not open '{}' as target", param.path.string()));

    reader = std::make_shared<ReadHandler>(fd_target);
    writer = std::make_shared<WriteHandler>(fd_target);
    flusher = std::make_shared<FlushHandler>(fd_target);
  } else {
    reader = std::make_shared<DummyReadHandler>();
    writer = std::make_shared<DummyWriteHandler>();
    flusher = std::make_shared<DummyFlushHandler>();
  }

  discarder = std::make_shared<DummyDiscardHandler>();

  auto hs = std::map<ublk_cmd_op, std::shared_ptr<IUblkReqHandler>>{
      {
          UBLK_CMD_OP_READ,
          std::make_unique<CmdReadHandlerAdaptor>(
              std::make_unique<CmdReadHandler>(std::move(reader))),
      },
      {
          UBLK_CMD_OP_WRITE,
          std::make_unique<CmdWriteHandlerAdaptor>(
              std::make_unique<CmdWriteHandler>(std::move(writer))),
      },
      {
          UBLK_CMD_OP_FLUSH,
          std::make_unique<CmdFlushHandlerAdaptor>(
              std::make_unique<CmdFlushHandler>(std::move(flusher))),
      },
      {
          UBLK_CMD_OP_DISCARD,
          std::make_unique<CmdDiscardHandlerAdaptor>(
              std::make_unique<CmdDiscardHandler>(std::move(discarder))),
      },
  };
  targets_[param.name] =
      std::make_unique<Target>(std::make_shared<ReqHandler>(std::move(hs)),
                               target_properties{param.capacity_sectors});
}

void Master::destroy(target_destroy_param const &param) {
  if (auto it = targets_.find(param.name); it != targets_.end()) {
    if (it->second.use_count() > 1)
      throw std::system_error(
          std::make_error_code(std::errc::device_or_resource_busy),
          fmt::format("target '{}' has {} references", param.name,
                      it->second.use_count() - 1));
    targets_.erase(it);
  }
}
