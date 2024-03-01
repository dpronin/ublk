#include "handler.hpp"

#include <cassert>
#include <cstddef>
#include <cstdint>
#include <cstdlib>

#include <fcntl.h>
#include <unistd.h>

#include <linux/ublkdrv/cmd.h>
#include <linux/ublkdrv/mapping.h>

#include <linux/types.h>

#include <format>
#include <stdexcept>
#include <thread>
#include <utility>

#include <spdlog/spdlog.h>

#include <boost/asio/buffer.hpp>
#include <boost/asio/posix/stream_descriptor.hpp>
#include <boost/asio/post.hpp>
#include <boost/asio/read.hpp>
#include <boost/asio/write.hpp>
#include <boost/system/detail/error_code.hpp>

#include "mm/mem_types.hpp"

#include "file.hpp"

namespace {

struct handler_ctx {
  std::unique_ptr<ublk::qublkcmd_t> qcmd;
  std::unique_ptr<ublk::IHandler<int(ublkdrv_cmd const &) noexcept>> handler;

  ublk::mm::uptrwd<int const> fd;
  std::unique_ptr<boost::asio::posix::stream_descriptor> sd;

  uint32_t cmds;
  uint32_t processed_cmds;
  uint32_t last_cmds;
};

ublk::mm::uptrwd<int const> fd_open(ublk::evpaths_t const &evpaths) {
  auto fd = ublk::mm::uptrwd<int const>{};

  if (auto it = evpaths.find(UBLKDRV_UIO_KERNEL_TO_USER_DIR_SUFFIX);
      it != evpaths.end()) {
    fd = ublk::open(it->second, O_RDWR);
    assert(fd);
  } else {
    throw std::invalid_argument(std::format(
        "no {} given as event path", UBLKDRV_UIO_KERNEL_TO_USER_DIR_SUFFIX));
  }

  return fd;
}

void async_read(std::shared_ptr<handler_ctx> ctx) {
  auto *p_ctx = ctx.get();
  boost::asio::async_read(
      *p_ctx->sd,
      boost::asio::mutable_buffer(&p_ctx->cmds, sizeof(p_ctx->cmds)),
      [ctx = std::move(ctx)](boost::system::error_code const &ec,
                             size_t bytes_read [[maybe_unused]]) mutable {
        if (ec) [[unlikely]] {
          spdlog::error("failed to async_read, ec {}", ec.value());
          return;
        } else {
          assert(bytes_read == sizeof(ctx->cmds));
        }

        ctx->processed_cmds = 0;
        for (ctx->cmds -= std::exchange(ctx->last_cmds, ctx->cmds); ctx->cmds;
             --ctx->cmds, ++ctx->processed_cmds) {
          auto cmd = ctx->qcmd->pop();
          for (; !cmd; cmd = ctx->qcmd->pop())
            std::this_thread::yield();
          boost::asio::post(
              ctx->sd->get_executor(), [ctx, cmd = std::move(*cmd)] {
                if (auto const r = ctx->handler->handle(cmd)) [[unlikely]]
                  std::abort();
              });
        }

        boost::asio::async_write(
            *ctx->sd,
            boost::asio::buffer(&ctx->processed_cmds,
                                sizeof(ctx->processed_cmds)),
            [ctx](boost::system::error_code const &ec,
                  size_t bytes_written [[maybe_unused]]) mutable {
              if (ec) [[unlikely]] {
                spdlog::error("failed to async_write, ec {}", ec.value());
                return;
              } else {
                assert(bytes_written == sizeof(ctx->processed_cmds));
              }
            });

        async_read(std::move(ctx));
      });
}

} // namespace

namespace ublk {

void async_start(
    evpaths_t const &evpaths, boost::asio::io_context &io_ctx,
    std::unique_ptr<qublkcmd_t> qcmd,
    std::unique_ptr<IHandler<int(ublkdrv_cmd const &) noexcept>> handler) {

  assert(qcmd);
  assert(handler);

  auto ctx = std::make_unique<handler_ctx>();

  ctx->qcmd = std::move(qcmd);
  ctx->handler = std::move(handler);

  ctx->fd = fd_open(evpaths);
  assert(ctx->fd);

  ctx->sd =
      std::make_unique<boost::asio::posix::stream_descriptor>(io_ctx, *ctx->fd);

  async_read(std::move(ctx));
}

} // namespace ublk
