#include "master.hpp"

#include <cstddef>
#include <cstring>

#include <fcntl.h>
#include <netlink/attr.h>
#include <sys/wait.h>
#include <unistd.h>

#include <linux/ublkdrv/genl.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <future>
#include <iterator>
#include <memory>
#include <stdexcept>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

#include "cached_rw_handler.hpp"
#include "file.hpp"
#include "genl.hpp"
#include "mem_types.hpp"
#include "req_handler.hpp"
#include "rw_handler_interface.hpp"
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
#include "flat_lru_cache.hpp"
#include "flush_handler_composite.hpp"
#include "flush_handler_interface.hpp"
#include "read_handler.hpp"
#include "read_handler_interface.hpp"
#include "rw_handler.hpp"
#include "sector.hpp"
#include "target.hpp"
#include "target_create_param.hpp"
#include "ublk_req_handler_interface.hpp"
#include "utility.hpp"
#include "write_handler.hpp"
#include "write_handler_interface.hpp"

#include "null/discard_handler.hpp"
#include "null/flush_handler.hpp"
#include "null/read_handler.hpp"
#include "null/write_handler.hpp"

#include "inmem/read_handler.hpp"
#include "inmem/target.hpp"
#include "inmem/write_handler.hpp"

#include "def/flush_handler.hpp"
#include "def/read_handler.hpp"
#include "def/target.hpp"
#include "def/write_handler.hpp"

#include "raid0/read_handler.hpp"
#include "raid0/target.hpp"
#include "raid0/write_handler.hpp"

#include "raid1/read_handler.hpp"
#include "raid1/target.hpp"
#include "raid1/write_handler.hpp"

#include "raid4/read_handler.hpp"
#include "raid4/target.hpp"
#include "raid4/write_handler.hpp"

#include "raid5/cached_target.hpp"
#include "raid5/read_handler.hpp"
#include "raid5/target.hpp"
#include "raid5/write_handler.hpp"

using namespace ublk;

namespace {

struct handlers_ops {
  std::shared_ptr<IReadHandler> reader;
  std::shared_ptr<IWriteHandler> writer;
  std::shared_ptr<IFlushHandler> flusher;
};

auto backend_device_open(std::filesystem::path const &path) {
  return open(path, O_RDWR | O_CREAT | O_DIRECT,
              S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
}

handlers_ops make_default_ops(uint64_t cache_len_sectors,
                              uptrwd<int const> fd) {
  auto target = std::make_shared<def::Target>(std::move(fd));

  auto rw_handler = std::unique_ptr<IRWHandler>{};

  rw_handler =
      std::make_unique<RWHandler>(std::make_shared<def::ReadHandler>(target),
                                  std::make_shared<def::WriteHandler>(target));
  if (auto cache = flat_lru_cache<uint64_t, std::byte>::create(
          cache_len_sectors, kSectorSz)) {
    rw_handler = std::make_unique<CachedRWHandler>(std::move(cache),
                                                   std::move(rw_handler));
  }

  auto sp_rw_handler = std::shared_ptr{std::move(rw_handler)};

  return {
      .reader = std::make_shared<ReadHandler>(sp_rw_handler),
      .writer = std::make_shared<WriteHandler>(sp_rw_handler),
      .flusher = std::make_shared<def::FlushHandler>(target),
  };
}

handlers_ops make_raid0_ops(uint64_t strip_sz, uint64_t cache_len_strips,
                            std::vector<handlers_ops> handlers) {
  std::vector<std::shared_ptr<IRWHandler>> rw_handlers;
  std::ranges::transform(std::move(handlers), std::back_inserter(rw_handlers),
                         [](auto &&ops) {
                           return std::make_shared<RWHandler>(
                               std::move(ops.reader), std::move(ops.writer));
                         });

  std::vector<std::shared_ptr<IFlushHandler>> flushers;
  std::ranges::transform(std::move(handlers), std::back_inserter(flushers),
                         [](auto &&ops) { return std::move(ops.flusher); });

  auto target =
      std::make_shared<raid0::Target>(strip_sz, std::move(rw_handlers));

  auto rw_handler = std::unique_ptr<IRWHandler>{};

  rw_handler = std::make_unique<RWHandler>(
      std::make_shared<raid0::ReadHandler>(target),
      std::make_shared<raid0::WriteHandler>(target));
  if (auto cache = flat_lru_cache<uint64_t, std::byte>::create(cache_len_strips,
                                                               strip_sz))
    rw_handler = std::make_unique<CachedRWHandler>(std::move(cache),
                                                   std::move(rw_handler));

  auto sp_rw_handler = std::shared_ptr{std::move(rw_handler)};

  return {
      .reader = std::make_shared<ReadHandler>(sp_rw_handler),
      .writer = std::make_shared<WriteHandler>(sp_rw_handler),
      .flusher = std::make_shared<FlushHandlerComposite>(std::move(flushers)),
  };
}

handlers_ops make_raid0_ops(uint64_t strip_sz, uint64_t cache_len_strips,
                            std::vector<uptrwd<const int>> fds) {
  std::vector<handlers_ops> default_hopss;
  std::ranges::transform(
      std::move(fds), std::back_inserter(default_hopss),
      [](auto &&fd) { return make_default_ops(0, std::move(fd)); });

  return make_raid0_ops(strip_sz, cache_len_strips, std::move(default_hopss));
}

handlers_ops make_raid0_ops(target_raid0_cfg const &raid0) {
  std::vector<uptrwd<int const>> fd_targets;
  std::ranges::transform(raid0.paths, std::back_inserter(fd_targets),
                         backend_device_open);
  return make_raid0_ops(sectors_to_bytes(raid0.strip_len_sectors),
                        raid0.cache_len_strips, std::move(fd_targets));
}

handlers_ops make_raid1_ops(uint64_t cache_len_sectors,
                            std::vector<handlers_ops> handlers) {
  std::vector<std::shared_ptr<IRWHandler>> rw_handlers;
  std::ranges::transform(std::move(handlers), std::back_inserter(rw_handlers),
                         [](auto &&ops) {
                           return std::make_shared<RWHandler>(
                               std::move(ops.reader), std::move(ops.writer));
                         });

  std::vector<std::shared_ptr<IFlushHandler>> flushers;
  std::ranges::transform(std::move(handlers), std::back_inserter(flushers),
                         [](auto &&ops) { return std::move(ops.flusher); });

  auto target = std::make_shared<raid1::Target>(std::move(rw_handlers));

  auto rw_handler = std::unique_ptr<IRWHandler>{};

  rw_handler = std::make_unique<RWHandler>(
      std::make_shared<raid1::ReadHandler>(target),
      std::make_shared<raid1::WriteHandler>(target));
  if (auto cache = flat_lru_cache<uint64_t, std::byte>::create(
          cache_len_sectors, kSectorSz))
    rw_handler = std::make_unique<CachedRWHandler>(std::move(cache),
                                                   std::move(rw_handler));

  auto sp_rw_handler = std::shared_ptr{std::move(rw_handler)};

  return {
      .reader = std::make_shared<ReadHandler>(sp_rw_handler),
      .writer = std::make_shared<WriteHandler>(sp_rw_handler),
      .flusher = std::make_shared<FlushHandlerComposite>(std::move(flushers)),
  };
}

handlers_ops make_raid1_ops(uint64_t cache_len_sectors,
                            std::vector<uptrwd<const int>> fds) {
  std::vector<handlers_ops> default_hopss;
  std::ranges::transform(
      std::move(fds), std::back_inserter(default_hopss),
      [](auto &&fd) { return make_default_ops(0, std::move(fd)); });

  return make_raid1_ops(cache_len_sectors, std::move(default_hopss));
}

handlers_ops make_raid1_ops(target_raid1_cfg const &raid1) {
  std::vector<uptrwd<int const>> fd_targets;
  std::ranges::transform(raid1.paths, std::back_inserter(fd_targets),
                         backend_device_open);
  return make_raid1_ops(raid1.cache_len_sectors, std::move(fd_targets));
}

handlers_ops make_raid4_ops(uint64_t strip_sz, uint64_t cache_len_stripes,
                            std::vector<uptrwd<int const>> fds) {
  assert(!(fds.size() < 2));

  std::vector<handlers_ops> default_hopss;
  std::ranges::transform(
      std::move(fds), std::back_inserter(default_hopss),
      [](auto &&fd) { return make_default_ops(0, std::move(fd)); });

  std::vector<std::shared_ptr<IRWHandler>> rw_handlers;
  std::ranges::transform(std::move(default_hopss),
                         std::back_inserter(rw_handlers), [](auto &&ops) {
                           return std::make_shared<RWHandler>(
                               std::move(ops.reader), std::move(ops.writer));
                         });

  std::vector<std::shared_ptr<IFlushHandler>> flushers;
  std::ranges::transform(std::move(default_hopss), std::back_inserter(flushers),
                         [](auto &&ops) { return std::move(ops.flusher); });

  auto target =
      std::make_shared<raid4::Target>(strip_sz, std::move(rw_handlers));

  auto rw_handler = std::unique_ptr<IRWHandler>{};

  auto const stripe_data_sz{strip_sz * (fds.size() - 1)};

  rw_handler = std::make_unique<RWHandler>(
      std::make_shared<raid4::ReadHandler>(target),
      std::make_shared<raid4::WriteHandler>(target));
  if (auto cache = flat_lru_cache<uint64_t, std::byte>::create(
          cache_len_stripes, stripe_data_sz))
    rw_handler = std::make_unique<CachedRWHandler>(std::move(cache),
                                                   std::move(rw_handler));

  auto sp_rw_handler = std::shared_ptr{std::move(rw_handler)};

  return {
      .reader = std::make_shared<ReadHandler>(sp_rw_handler),
      .writer = std::make_shared<WriteHandler>(sp_rw_handler),
      .flusher = std::make_shared<FlushHandlerComposite>(std::move(flushers)),
  };
}

handlers_ops make_raid4_ops(target_raid4_cfg const &raid4) {
  std::vector<uptrwd<int const>> fd_targets;
  std::ranges::transform(raid4.data_paths, std::back_inserter(fd_targets),
                         backend_device_open);
  fd_targets.push_back(backend_device_open(raid4.parity_path));
  return make_raid4_ops(sectors_to_bytes(raid4.strip_len_sectors),
                        raid4.cache_len_stripes, std::move(fd_targets));
}

handlers_ops make_raid5_ops(uint64_t strip_sz, uint64_t cache_len_stripes,
                            std::vector<uptrwd<int const>> fds) {
  assert(!(fds.size() < 2));

  std::vector<handlers_ops> default_hopss;
  std::ranges::transform(
      std::move(fds), std::back_inserter(default_hopss),
      [](auto &&fd) { return make_default_ops(0, std::move(fd)); });

  std::vector<std::shared_ptr<IRWHandler>> rw_handlers;
  std::ranges::transform(std::move(default_hopss),
                         std::back_inserter(rw_handlers), [](auto &&ops) {
                           return std::make_shared<RWHandler>(
                               std::move(ops.reader), std::move(ops.writer));
                         });

  std::vector<std::shared_ptr<IFlushHandler>> flushers;
  std::ranges::transform(std::move(default_hopss), std::back_inserter(flushers),
                         [](auto &&ops) { return std::move(ops.flusher); });

  auto target = std::shared_ptr<raid5::Target>{};

  if (auto cache = flat_lru_cache<uint64_t, std::byte>::create(
          cache_len_stripes, strip_sz * fds.size())) {
    target = std::make_shared<raid5::CachedTarget>(strip_sz, std::move(cache),
                                                   std::move(rw_handlers));
  } else {
    target = std::make_shared<raid5::Target>(strip_sz, std::move(rw_handlers));
  }

  return {
      .reader = std::make_shared<raid5::ReadHandler>(target),
      .writer = std::make_shared<raid5::WriteHandler>(target),
      .flusher = std::make_shared<FlushHandlerComposite>(std::move(flushers)),
  };
}

handlers_ops make_raid5_ops(target_raid5_cfg const &raid5) {
  std::vector<uptrwd<int const>> fd_targets;
  std::ranges::transform(raid5.paths, std::back_inserter(fd_targets),
                         backend_device_open);
  return make_raid5_ops(sectors_to_bytes(raid5.strip_len_sectors),
                        raid5.cache_len_stripes, std::move(fd_targets));
}

} // namespace

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
        std::format("'{}' target does not exist", param.target_name));

  auto nl_sock = genl::sock_open();
  auto msg = genl::msg_alloc();

  genlmsg_put(msg.get(), 0, 0, genl::resolve(*nl_sock), 0, 0,
              UBLKDRV_GENL_BDEV_CMD_CREATE, 1);

  nla_put_string(msg.get(), UBLKDRV_GENL_BDEV_ATTR_NAME_SUFFIX,
                 param.bdev_suffix.c_str());
  nla_put_u64(msg.get(), UBLKDRV_GENL_BDEV_ATTR_CAPACITY_SECTORS,
              target->properties().capacity_sectors);

  if (param.read_only)
    nla_put_flag(msg.get(), UBLKDRV_GENL_BDEV_ATTR_READ_ONLY);

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
    throw std::runtime_error(std::format(
        "{}: failed to create child, reason: {}", getpid(), strerror(errno)));
  }
}

void Master::unmap(bdev_unmap_param const &param) {
  auto nl_sock = genl::sock_open();
  auto msg = genl::msg_alloc();

  genlmsg_put(msg.get(), 0, 0, genl::resolve(*nl_sock), 0, 0,
              UBLKDRV_GENL_BDEV_CMD_DESTROY, 1);
  nla_put_string(msg.get(), UBLKDRV_GENL_BDEV_ATTR_NAME_SUFFIX,
                 param.bdev_suffix.data());

  genl::auto_send(*nl_sock, *msg);
}

void Master::create(target_create_param const &param) {
  if (targets_.contains(param.name))
    throw std::invalid_argument(
        std::format("{} target already exists", param.name));

  auto reader = std::shared_ptr<IReadHandler>{};
  auto writer = std::shared_ptr<IWriteHandler>{};
  auto flusher = std::shared_ptr<IFlushHandler>{};
  auto discarder = std::shared_ptr<IDiscardHandler>{};

  std::visit(
      overloaded{
          [&](target_null_cfg const &) {},
          [&](target_inmem_cfg const &) {
            uint64_t const mem_sz{sectors_to_bytes(param.capacity_sectors)};
            auto target = std::make_shared<inmem::Target>(
                get_unique_bytes_generator(mem_sz)(), mem_sz);
            reader = std::make_shared<inmem::ReadHandler>(target);
            writer = std::make_shared<inmem::WriteHandler>(target);
          },
          [&](target_default_cfg const &def) {
            auto ops = make_default_ops(def.cache_len_sectors,
                                        backend_device_open(def.path));
            reader = std::move(ops.reader);
            writer = std::move(ops.writer);
            flusher = std::move(ops.flusher);
          },
          [&](target_raid0_cfg const &raid0) {
            auto ops = make_raid0_ops(raid0);
            reader = std::move(ops.reader);
            writer = std::move(ops.writer);
            flusher = std::move(ops.flusher);
          },
          [&](target_raid1_cfg const &raid1) {
            auto ops = make_raid1_ops(raid1);
            reader = std::move(ops.reader);
            writer = std::move(ops.writer);
            flusher = std::move(ops.flusher);
          },
          [&](target_raid4_cfg const &raid4) {
            auto ops = make_raid4_ops(raid4);
            reader = std::move(ops.reader);
            writer = std::move(ops.writer);
            flusher = std::move(ops.flusher);
          },
          [&](target_raid5_cfg const &raid5) {
            auto ops = make_raid5_ops(raid5);
            reader = std::move(ops.reader);
            writer = std::move(ops.writer);
            flusher = std::move(ops.flusher);
          },
          [&](target_raid10_cfg const &raid10) {
            std::vector<handlers_ops> raid1s_ops;
            std::ranges::transform(
                raid10.raid1s, std::back_inserter(raid1s_ops),
                [](auto const &raid1) { return make_raid1_ops(raid1); });

            auto ops =
                make_raid0_ops(sectors_to_bytes(raid10.strip_len_sectors),
                               raid10.cache_len_strips, std::move(raid1s_ops));

            reader = std::move(ops.reader);
            writer = std::move(ops.writer);
            flusher = std::move(ops.flusher);
          },
          [&](target_raid40_cfg const &raid40) {
            std::vector<handlers_ops> raid4s_ops;
            std::ranges::transform(
                raid40.raid4s, std::back_inserter(raid4s_ops),
                [&](auto const &raid4) { return make_raid4_ops(raid4); });

            auto ops =
                make_raid0_ops(sectors_to_bytes(raid40.strip_len_sectors),
                               raid40.cache_len_strips, std::move(raid4s_ops));

            reader = std::move(ops.reader);
            writer = std::move(ops.writer);
            flusher = std::move(ops.flusher);
          },
          [&](target_raid50_cfg const &raid50) {
            std::vector<handlers_ops> raid5s_ops;
            std::ranges::transform(
                raid50.raid5s, std::back_inserter(raid5s_ops),
                [&](auto const &raid5) { return make_raid5_ops(raid5); });

            auto ops =
                make_raid0_ops(sectors_to_bytes(raid50.strip_len_sectors),
                               raid50.cache_len_strips, std::move(raid5s_ops));

            reader = std::move(ops.reader);
            writer = std::move(ops.writer);
            flusher = std::move(ops.flusher);
          },
      },
      param.target_cfg);

  if (!reader)
    reader = std::make_shared<null::ReadHandler>();

  if (!writer)
    writer = std::make_shared<null::WriteHandler>();

  if (!flusher)
    flusher = std::make_shared<null::FlushHandler>();

  if (!discarder)
    discarder = std::make_shared<null::DiscardHandler>();

  auto hs = std::map<ublkdrv_cmd_op, std::shared_ptr<IUblkReqHandler>>{
      {
          UBLKDRV_CMD_OP_READ,
          std::make_unique<CmdReadHandlerAdaptor>(
              std::make_unique<CmdReadHandler>(std::move(reader))),
      },
      {
          UBLKDRV_CMD_OP_WRITE,
          std::make_unique<CmdWriteHandlerAdaptor>(
              std::make_unique<CmdWriteHandler>(std::move(writer))),
      },
      {
          UBLKDRV_CMD_OP_FLUSH,
          std::make_unique<CmdFlushHandlerAdaptor>(
              std::make_unique<CmdFlushHandler>(std::move(flusher))),
      },
      {
          UBLKDRV_CMD_OP_DISCARD,
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
          std::format("target '{}' has {} references", param.name,
                      it->second.use_count() - 1));
    targets_.erase(it);
  }
}
