#include "handler.hpp"

#include <cerrno>
#include <cstddef>
#include <cstdlib>

#include <fcntl.h>
#include <functional>
#include <stdexcept>
#include <type_traits>
#include <unistd.h>

#include <sys/epoll.h>

#include <linux/ublk/cmd.h>
#include <linux/ublk/mapping.h>

#include <linux/types.h>

#include <array>
#include <concepts>
#include <limits>
#include <thread>
#include <utility>

#include <spdlog/spdlog.h>

#include "epoll.hpp"
#include "file.hpp"
#include "mem.hpp"

namespace {

enum {
  EV_FD_EPOLL,
  EV_FD_LISTEN,
  EV_FD_NOTIFY_REQ = EV_FD_LISTEN,
  EV_FDS_QTY,
};

auto fds_open(ublk::evpaths_t const &evpaths) {
  std::array<ublk::uptrwd<int const>, EV_FDS_QTY> fds;

  if (auto it = evpaths.find(UBLK_UIO_KERNEL_TO_USER_DIR_SUFFIX);
      it != evpaths.end()) {

    fds[EV_FD_LISTEN] = ublk::open(it->second, O_RDWR);

    struct epoll_event ev {
      .events = EPOLLIN, .data = {.fd = *fds[EV_FD_LISTEN]},
    };

    fds[EV_FD_EPOLL] = ublk::epoll_create1(0);

    if (epoll_ctl(*fds[EV_FD_EPOLL], EPOLL_CTL_ADD, *fds[EV_FD_LISTEN], &ev) <
        0) {
      throw std::runtime_error(fmt::format(
          "couldn't add FD of {} for listening to poll", it->second.string()));
    }
  } else {
    throw std::invalid_argument(fmt::format(
        "no {} given as event path", UBLK_UIO_KERNEL_TO_USER_DIR_SUFFIX));
  }

  return fds;
}

} // namespace

namespace ublk {

int handler(qublkcmd_t &qcmd, evpaths_t const &evpaths,
            IHandler<int(ublk_cmd) noexcept> &handler) {

  auto const fds = fds_open(evpaths);

  uint32_t last_cmds{0};

  for (struct epoll_event event;;) {
    spdlog::debug("is working");

    auto const nfds = epoll_wait(*fds[EV_FD_EPOLL], &event, 1, -1);
    if (nfds == -1 && EINTR != errno) [[unlikely]]
      return EXIT_FAILURE;

    for (int i = 0; i < std::min(1, nfds); ++i) {
      if (event.data.fd != *fds[EV_FD_LISTEN]) [[unlikely]]
        continue;

      uint32_t cmds;
      if (auto const r = read(*fds[EV_FD_LISTEN], &cmds, sizeof(cmds)); r != 4)
          [[unlikely]]
        return EXIT_FAILURE;

      for (cmds -= std::exchange(last_cmds, cmds); cmds; --cmds) {
        auto cmd = qcmd.pop();
        for (; !cmd; cmd = qcmd.pop())
          std::this_thread::yield();

        if (uint32_t processed_cmds = 1;
            4 != write(*fds[EV_FD_NOTIFY_REQ], &processed_cmds,
                       sizeof(processed_cmds))) [[unlikely]] {

          return EXIT_FAILURE;
        }

        if (auto const r = handler.handle(*cmd)) [[unlikely]] {
          return r;
        }
      }
    }
  }

  return EXIT_SUCCESS;
}

} // namespace ublk
