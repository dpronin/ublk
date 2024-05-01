#include "master.hpp"

#include <cstddef>
#include <cstring>

#include <fcntl.h>
#include <netlink/attr.h>
#include <ranges>
#include <sys/wait.h>
#include <unistd.h>

#include <linux/ublkdrv/genl.h>

#include <algorithm>
#include <filesystem>
#include <format>
#include <future>
#include <iterator>
#include <limits>
#include <memory>
#include <optional>
#include <stdexcept>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

#include <boost/asio/io_context.hpp>

#include "mm/mem.hpp"
#include "mm/mem_types.hpp"

#include "sys/file.hpp"
#include "sys/genl.hpp"

#include "utils/utility.hpp"

#include "cache/rw_handler.hpp"

#include "cmd_handler_factory.hpp"
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
#include "flq_submitter_composite.hpp"
#include "flq_submitter_interface.hpp"
#include "rdq_submitter.hpp"
#include "rdq_submitter_interface.hpp"
#include "rw_handler.hpp"
#include "sector.hpp"
#include "target.hpp"
#include "target_create_param.hpp"
#include "ublk_req_handler_interface.hpp"
#include "wrq_submitter.hpp"
#include "wrq_submitter_interface.hpp"

#include "null/discard_handler.hpp"
#include "null/flq_submitter.hpp"
#include "null/rdq_submitter.hpp"
#include "null/wrq_submitter.hpp"

#include "inmem/rdq_submitter.hpp"
#include "inmem/target.hpp"
#include "inmem/wrq_submitter.hpp"

#include "def/flq_submitter.hpp"
#include "def/rdq_submitter.hpp"
#include "def/target.hpp"
#include "def/wrq_submitter.hpp"

#include "raid0/rdq_submitter.hpp"
#include "raid0/target.hpp"
#include "raid0/wrq_submitter.hpp"

#include "raid1/rdq_submitter.hpp"
#include "raid1/target.hpp"
#include "raid1/wrq_submitter.hpp"

#include "raid4/rdq_submitter.hpp"
#include "raid4/target.hpp"
#include "raid4/wrq_submitter.hpp"

#include "raid5/rdq_submitter.hpp"
#include "raid5/target.hpp"
#include "raid5/wrq_submitter.hpp"

using namespace ublk;

namespace {

struct handlers_ops {
  std::shared_ptr<IRDQSubmitter> reader;
  std::shared_ptr<IWRQSubmitter> writer;
  std::shared_ptr<IFLQSubmitter> flusher;
};

auto backend_device_open(std::filesystem::path const &path) {
  return sys::open(path, O_RDWR | O_CREAT | O_DIRECT,
                   S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
}

handlers_ops make_default_ops(boost::asio::io_context &io_ctx,
                              std::optional<cache_cfg> const &cache_cfg,
                              mm::uptrwd<int const> fd) {
  auto target{std::make_shared<def::Target>(io_ctx, std::move(fd))};

  auto rw_handler{std::unique_ptr<IRWHandler>{}};

  rw_handler =
      std::make_unique<RWHandler>(std::make_shared<def::RDQSubmitter>(target),
                                  std::make_shared<def::WRQSubmitter>(target));

  if (cache_cfg && cache_cfg->len_sectors) {
    rw_handler = std::make_unique<cache::RWHandler>(
        cache_cfg->len_sectors, std::move(rw_handler),
        cache_cfg->write_through_enable);
  }

  auto sp_rw_handler{std::shared_ptr{std::move(rw_handler)}};

  return {
      .reader = std::make_shared<RDQSubmitter>(sp_rw_handler),
      .writer = std::make_shared<WRQSubmitter>(sp_rw_handler),
      .flusher = std::make_shared<def::FLQSubmitter>(target),
  };
}

handlers_ops make_raid0_ops(uint64_t strip_sz,
                            std::optional<cache_cfg> const &cache_cfg,
                            std::vector<handlers_ops> handlers) {
  std::vector<std::shared_ptr<IRWHandler>> rw_handlers;
  std::ranges::transform(std::move(handlers), std::back_inserter(rw_handlers),
                         [](auto &&ops) {
                           return std::make_shared<RWHandler>(
                               std::move(ops.reader), std::move(ops.writer));
                         });

  std::vector<std::shared_ptr<IFLQSubmitter>> flushers;
  std::ranges::transform(std::move(handlers), std::back_inserter(flushers),
                         [](auto &&ops) { return std::move(ops.flusher); });

  auto target{
      std::make_shared<raid0::Target>(strip_sz, std::move(rw_handlers)),
  };

  auto rw_handler{std::unique_ptr<IRWHandler>{}};

  rw_handler = std::make_unique<RWHandler>(
      std::make_shared<raid0::RDQSubmitter>(target),
      std::make_shared<raid0::WRQSubmitter>(target));

  if (cache_cfg && cache_cfg->len_sectors) {
    rw_handler = std::make_unique<cache::RWHandler>(
        cache_cfg->len_sectors, std::move(rw_handler),
        cache_cfg->write_through_enable);
  }

  auto sp_rw_handler{std::shared_ptr{std::move(rw_handler)}};

  return {
      .reader = std::make_shared<RDQSubmitter>(sp_rw_handler),
      .writer = std::make_shared<WRQSubmitter>(sp_rw_handler),
      .flusher = std::make_shared<FLQSubmitterComposite>(std::move(flushers)),
  };
}

handlers_ops make_raid0_ops(boost::asio::io_context &io_ctx, uint64_t strip_sz,
                            std::optional<cache_cfg> const &cache_cfg,
                            std::vector<mm::uptrwd<const int>> fds) {
  std::vector<handlers_ops> default_hopss;
  std::ranges::transform(
      std::move(fds), std::back_inserter(default_hopss),
      [&](auto &&fd) { return make_default_ops(io_ctx, {}, std::move(fd)); });

  return make_raid0_ops(strip_sz, cache_cfg, std::move(default_hopss));
}

handlers_ops make_raid0_ops(boost::asio::io_context &io_ctx,
                            std::optional<cache_cfg> const &cache_cfg,
                            target_raid0_cfg const &raid0) {
  std::vector<mm::uptrwd<int const>> fd_targets;
  std::ranges::transform(raid0.paths, std::back_inserter(fd_targets),
                         backend_device_open);
  return make_raid0_ops(io_ctx, sectors_to_bytes(raid0.strip_len_sectors),
                        cache_cfg, std::move(fd_targets));
}

handlers_ops make_raid1_ops(uint64_t read_strip_len_sectors,
                            std::optional<cache_cfg> const &cache_cfg,
                            std::vector<handlers_ops> handlers) {
  std::vector<std::shared_ptr<IRWHandler>> rw_handlers;
  std::ranges::transform(std::move(handlers), std::back_inserter(rw_handlers),
                         [](auto &&ops) {
                           return std::make_shared<RWHandler>(
                               std::move(ops.reader), std::move(ops.writer));
                         });

  std::vector<std::shared_ptr<IFLQSubmitter>> flushers;
  std::ranges::transform(std::move(handlers), std::back_inserter(flushers),
                         [](auto &&ops) { return std::move(ops.flusher); });

  if (read_strip_len_sectors >
      bytes_to_sectors(std::numeric_limits<uint64_t>::max()))
    throw std::overflow_error(std::format(
        "read_strip_len_sectors {} is too huge", read_strip_len_sectors));

  auto read_strip_sz{sectors_to_bytes(read_strip_len_sectors)};
  if (0 == read_strip_sz)
    read_strip_sz = sectors_to_bytes(
        bytes_to_sectors(std::numeric_limits<uint64_t>::max()));

  auto target{
      std::make_shared<raid1::Target>(read_strip_sz, std::move(rw_handlers)),
  };

  auto rw_handler{std::unique_ptr<IRWHandler>{}};

  rw_handler = std::make_unique<RWHandler>(
      std::make_shared<raid1::RDQSubmitter>(target),
      std::make_shared<raid1::WRQSubmitter>(target));

  if (cache_cfg && cache_cfg->len_sectors) {
    rw_handler = std::make_unique<cache::RWHandler>(
        cache_cfg->len_sectors, std::move(rw_handler),
        cache_cfg->write_through_enable);
  }

  auto sp_rw_handler{std::shared_ptr{std::move(rw_handler)}};

  return {
      .reader = std::make_shared<RDQSubmitter>(sp_rw_handler),
      .writer = std::make_shared<WRQSubmitter>(sp_rw_handler),
      .flusher = std::make_shared<FLQSubmitterComposite>(std::move(flushers)),
  };
}

handlers_ops make_raid1_ops(boost::asio::io_context &io_ctx,
                            uint64_t read_strip_len_sectors,
                            std::optional<cache_cfg> const &cache_cfg,
                            std::vector<mm::uptrwd<const int>> fds) {
  std::vector<handlers_ops> default_hopss;
  std::ranges::transform(
      std::move(fds), std::back_inserter(default_hopss),
      [&](auto &&fd) { return make_default_ops(io_ctx, {}, std::move(fd)); });

  return make_raid1_ops(read_strip_len_sectors, cache_cfg,
                        std::move(default_hopss));
}

handlers_ops make_raid1_ops(boost::asio::io_context &io_ctx,
                            std::optional<cache_cfg> const &cache_cfg,
                            target_raid1_cfg const &raid1) {
  std::vector<mm::uptrwd<int const>> fd_targets;
  std::ranges::transform(raid1.paths, std::back_inserter(fd_targets),
                         backend_device_open);
  return make_raid1_ops(io_ctx, raid1.read_strip_len_sectors, cache_cfg,
                        std::move(fd_targets));
}

handlers_ops make_raid4_ops(boost::asio::io_context &io_ctx, uint64_t strip_sz,
                            std::optional<cache_cfg> const &cache_cfg,
                            std::vector<mm::uptrwd<int const>> fds) {
  std::vector<handlers_ops> default_hopss;
  std::ranges::transform(
      std::move(fds), std::back_inserter(default_hopss),
      [&](auto &&fd) { return make_default_ops(io_ctx, {}, std::move(fd)); });

  std::vector<std::shared_ptr<IRWHandler>> rw_handlers;
  std::ranges::transform(std::move(default_hopss),
                         std::back_inserter(rw_handlers), [](auto &&ops) {
                           return std::make_shared<RWHandler>(
                               std::move(ops.reader), std::move(ops.writer));
                         });

  std::vector<std::shared_ptr<IFLQSubmitter>> flushers;
  std::ranges::transform(std::move(default_hopss), std::back_inserter(flushers),
                         [](auto &&ops) { return std::move(ops.flusher); });

  auto target{
      std::make_shared<raid4::Target>(strip_sz, std::move(rw_handlers)),
  };

  auto rw_handler{std::unique_ptr<IRWHandler>{}};

  rw_handler = std::make_unique<RWHandler>(
      std::make_shared<raid4::RDQSubmitter>(target),
      std::make_shared<raid4::WRQSubmitter>(target));

  if (cache_cfg && cache_cfg->len_sectors) {
    rw_handler = std::make_unique<cache::RWHandler>(
        cache_cfg->len_sectors, std::move(rw_handler),
        cache_cfg->write_through_enable);
  }

  auto sp_rw_handler{std::shared_ptr{std::move(rw_handler)}};

  return {
      .reader = std::make_shared<RDQSubmitter>(sp_rw_handler),
      .writer = std::make_shared<WRQSubmitter>(sp_rw_handler),
      .flusher = std::make_shared<FLQSubmitterComposite>(std::move(flushers)),
  };
}

handlers_ops make_raid4_ops(boost::asio::io_context &io_ctx,
                            std::optional<cache_cfg> const &cache_cfg,
                            target_raid4_cfg const &raid4) {
  std::vector<mm::uptrwd<int const>> fd_targets;
  std::ranges::transform(raid4.data_paths, std::back_inserter(fd_targets),
                         backend_device_open);
  fd_targets.push_back(backend_device_open(raid4.parity_path));
  return make_raid4_ops(io_ctx, sectors_to_bytes(raid4.strip_len_sectors),
                        cache_cfg, std::move(fd_targets));
}

handlers_ops make_raid5_ops(boost::asio::io_context &io_ctx, uint64_t strip_sz,
                            std::optional<cache_cfg> const &cache_cfg,
                            std::vector<mm::uptrwd<int const>> fds) {
  std::vector<handlers_ops> default_hopss;
  std::ranges::transform(
      std::move(fds), std::back_inserter(default_hopss),
      [&](auto &&fd) { return make_default_ops(io_ctx, {}, std::move(fd)); });

  std::vector<std::shared_ptr<IRWHandler>> rw_handlers;
  std::ranges::transform(std::move(default_hopss),
                         std::back_inserter(rw_handlers), [](auto &&ops) {
                           return std::make_shared<RWHandler>(
                               std::move(ops.reader), std::move(ops.writer));
                         });

  std::vector<std::shared_ptr<IFLQSubmitter>> flushers;
  std::ranges::transform(std::move(default_hopss), std::back_inserter(flushers),
                         [](auto &&ops) { return std::move(ops.flusher); });

  auto target{
      std::make_shared<raid5::Target>(strip_sz, std::move(rw_handlers)),
  };

  auto rw_handler{std::unique_ptr<IRWHandler>{}};

  rw_handler = std::make_unique<RWHandler>(
      std::make_shared<raid5::RDQSubmitter>(target),
      std::make_shared<raid5::WRQSubmitter>(target));

  if (cache_cfg && cache_cfg->len_sectors) {
    rw_handler = std::make_unique<cache::RWHandler>(
        cache_cfg->len_sectors, std::move(rw_handler),
        cache_cfg->write_through_enable);
  }

  auto sp_rw_handler{std::shared_ptr{std::move(rw_handler)}};

  return {
      .reader = std::make_shared<RDQSubmitter>(sp_rw_handler),
      .writer = std::make_shared<WRQSubmitter>(sp_rw_handler),
      .flusher = std::make_shared<FLQSubmitterComposite>(std::move(flushers)),
  };
}

handlers_ops make_raid5_ops(boost::asio::io_context &io_ctx,
                            std::optional<cache_cfg> const &cache_cfg,
                            target_raid5_cfg const &raid5) {
  auto fd_targets{std::vector<mm::uptrwd<int const>>{}};
  std::ranges::transform(raid5.paths, std::back_inserter(fd_targets),
                         backend_device_open);
  return make_raid5_ops(io_ctx, sectors_to_bytes(raid5.strip_len_sectors),
                        cache_cfg, std::move(fd_targets));
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
  auto target{std::shared_ptr<Target>{}};

  if (auto it{targets_.find(param.target_name)}; targets_.end() != it)
    target = it->second;

  if (!target)
    throw std::invalid_argument(
        std::format("'{}' target does not exist", param.target_name));

  auto nl_sock{sys::genl::sock_open()};
  auto msg{sys::genl::msg_alloc()};

  genlmsg_put(msg.get(), 0, 0, sys::genl::resolve(*nl_sock), 0, 0,
              UBLKDRV_GENL_BDEV_CMD_CREATE, 1);

  nla_put_string(msg.get(), UBLKDRV_GENL_BDEV_ATTR_NAME_SUFFIX,
                 param.bdev_suffix.c_str());
  nla_put_u64(msg.get(), UBLKDRV_GENL_BDEV_ATTR_CAPACITY_SECTORS,
              target->properties().capacity_sectors);

  if (param.read_only)
    nla_put_flag(msg.get(), UBLKDRV_GENL_BDEV_ATTR_READ_ONLY);

  sys::genl::auto_send(*nl_sock, *msg);

  nl_sock.reset();

  if (auto const child_pid{fork()}; 0 == child_pid) {
    spdlog::set_pattern("[slave %P] [%^%l%$]: %v");
    spdlog::info("started: {}", param.target_name);
    slave::run(target->io_ctx(), {
                                     .bdev_suffix = param.bdev_suffix,
                                     .hfactory = target->req_hfactory(),
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
  auto nl_sock{sys::genl::sock_open()};
  auto msg{sys::genl::msg_alloc()};

  genlmsg_put(msg.get(), 0, 0, sys::genl::resolve(*nl_sock), 0, 0,
              UBLKDRV_GENL_BDEV_CMD_DESTROY, 1);
  nla_put_string(msg.get(), UBLKDRV_GENL_BDEV_ATTR_NAME_SUFFIX,
                 param.bdev_suffix.data());

  sys::genl::auto_send(*nl_sock, *msg);
}

void Master::create(target_create_param const &param) {
  if (targets_.contains(param.name))
    throw std::invalid_argument(
        std::format("{} target already exists", param.name));

  auto io_ctx{std::make_unique<boost::asio::io_context>()};

  auto reader{std::shared_ptr<IRDQSubmitter>{}};
  auto writer{std::shared_ptr<IWRQSubmitter>{}};
  auto flusher{std::shared_ptr<IFLQSubmitter>{}};
  auto discarder{std::shared_ptr<IDiscardHandler>{}};

  std::visit(
      overloaded{
          [&](target_null_cfg const &) {},
          [&](target_inmem_cfg const &) {
            uint64_t const mem_sz{sectors_to_bytes(param.capacity_sectors)};
            auto target{
                std::make_shared<inmem::Target>(
                    mm::get_unique_bytes_generator(kSectorSz, mem_sz)(),
                    mem_sz),
            };
            reader = std::make_shared<inmem::RDQSubmitter>(target);
            writer = std::make_shared<inmem::WRQSubmitter>(target);
          },
          [&](target_default_cfg const &def) {
            auto ops{
                make_default_ops(*io_ctx, param.cache,
                                 backend_device_open(def.path)),
            };
            reader = std::move(ops.reader);
            writer = std::move(ops.writer);
            flusher = std::move(ops.flusher);
          },
          [&](target_raid0_cfg const &raid0) {
            auto ops{make_raid0_ops(*io_ctx, param.cache, raid0)};
            reader = std::move(ops.reader);
            writer = std::move(ops.writer);
            flusher = std::move(ops.flusher);
          },
          [&](target_raid1_cfg const &raid1) {
            auto ops{make_raid1_ops(*io_ctx, param.cache, raid1)};
            reader = std::move(ops.reader);
            writer = std::move(ops.writer);
            flusher = std::move(ops.flusher);
          },
          [&](target_raid4_cfg const &raid4) {
            auto ops{make_raid4_ops(*io_ctx, param.cache, raid4)};
            reader = std::move(ops.reader);
            writer = std::move(ops.writer);
            flusher = std::move(ops.flusher);
          },
          [&](target_raid5_cfg const &raid5) {
            auto ops{make_raid5_ops(*io_ctx, param.cache, raid5)};
            reader = std::move(ops.reader);
            writer = std::move(ops.writer);
            flusher = std::move(ops.flusher);
          },
          [&](target_raid10_cfg const &raid10) {
            std::vector<handlers_ops> raid1s_ops;
            std::ranges::transform(raid10.raid1s,
                                   std::back_inserter(raid1s_ops),
                                   [&](auto const &raid1) {
                                     return make_raid1_ops(*io_ctx, {}, raid1);
                                   });

            auto ops{
                make_raid0_ops(sectors_to_bytes(raid10.strip_len_sectors),
                               param.cache, std::move(raid1s_ops)),
            };

            reader = std::move(ops.reader);
            writer = std::move(ops.writer);
            flusher = std::move(ops.flusher);
          },
          [&](target_raid40_cfg const &raid40) {
            std::vector<handlers_ops> raid4s_ops;
            std::ranges::transform(raid40.raid4s,
                                   std::back_inserter(raid4s_ops),
                                   [&](auto const &raid4) {
                                     return make_raid4_ops(*io_ctx, {}, raid4);
                                   });

            auto ops{
                make_raid0_ops(sectors_to_bytes(raid40.strip_len_sectors),
                               param.cache, std::move(raid4s_ops)),
            };

            reader = std::move(ops.reader);
            writer = std::move(ops.writer);
            flusher = std::move(ops.flusher);
          },
          [&](target_raid50_cfg const &raid50) {
            std::vector<handlers_ops> raid5s_ops;
            std::ranges::transform(raid50.raid5s,
                                   std::back_inserter(raid5s_ops),
                                   [&](auto const &raid5) {
                                     return make_raid5_ops(*io_ctx, {}, raid5);
                                   });

            auto ops{
                make_raid0_ops(sectors_to_bytes(raid50.strip_len_sectors),
                               param.cache, std::move(raid5s_ops)),
            };

            reader = std::move(ops.reader);
            writer = std::move(ops.writer);
            flusher = std::move(ops.flusher);
          },
      },
      param.target);

  if (!reader)
    reader = std::make_shared<null::RDQSubmitter>();

  if (!writer)
    writer = std::make_shared<null::WRQSubmitter>();

  if (!flusher)
    flusher = std::make_shared<null::FLQSubmitter>();

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
  targets_[param.name] = std::make_unique<Target>(
      std::move(io_ctx), std::make_shared<CmdHandlerFactory>(std::move(hs)),
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

std::list<std::pair<std::string, size_t>>
Master::list(targets_list_param const &) {
  /* clang-format off */
  auto const list_view{
        std::views::all(targets_)
      | std::views::transform([](auto const &item) -> std::pair<std::string, size_t> { return {item.first, item.second->properties().capacity_sectors}; }),
  };
  /* clang-format on */
#ifdef __clang__
  return std::ranges::to<std::list<std::pair<std::string, size_t>>>(list_view);
#else
  return {std::ranges::begin(list_view), std::ranges::end(list_view)};
#endif
}
