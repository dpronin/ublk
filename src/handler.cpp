#include "handler.hpp"

#include <cerrno>
#include <cstddef>
#include <cstdlib>

#include <fcntl.h>
#include <functional>
#include <type_traits>
#include <unistd.h>

#include <sys/epoll.h>

#include <linux/ublk/cellc.h>
#include <linux/ublk/cmd.h>
#include <linux/ublk/cmd_ack.h>
#include <linux/ublk/mapping.h>

#include <linux/types.h>

#include <array>
#include <concepts>
#include <limits>
#include <thread>
#include <utility>
#include <vector>

#include <spdlog/spdlog.h>

#include "file.hpp"
#include "mem.hpp"

namespace cfq {

int handler(qublkcmd_t &qcmd, evpaths_t const &evpaths,
            ICmdHandler<const ublk_cmd> &handler) {
  enum {
    EV_FD_EPOLL,
    EV_FD_LISTEN,
    EV_FD_NOTIFY_REQ = EV_FD_LISTEN,
    EV_FDS_QTY,
  };

  using pfd_t = std::unique_ptr<int const, decltype(pfdcloser)>;

  std::array<int, EV_FDS_QTY> fds;
  std::vector<pfd_t> pfds_guards;

  fds[EV_FD_EPOLL] = ::epoll_create1(0);
  if (fds[EV_FD_EPOLL] < 0)
    return EXIT_FAILURE;
  pfds_guards.push_back({&fds[EV_FD_EPOLL], pfdcloser});

  if (auto it = evpaths.find(UBLK_UIO_KERNEL_TO_USER_DIR_SUFFIX);
      it != evpaths.end()) {
    fds[EV_FD_LISTEN] = ::open(it->second.c_str(), O_RDWR);
    if (fds[EV_FD_LISTEN] < 0)
      return EXIT_FAILURE;
    pfds_guards.push_back({&fds[EV_FD_LISTEN], pfdcloser});

    struct epoll_event ev {
      .events = EPOLLIN, .data = {.fd = fds[EV_FD_LISTEN]},
    };

    if (epoll_ctl(fds[EV_FD_EPOLL], EPOLL_CTL_ADD, fds[EV_FD_LISTEN], &ev) < 0)
      return EXIT_FAILURE;
  } else {
    return EXIT_FAILURE;
  }

  uint32_t last_cmds{0};

  for (struct epoll_event event;;) {
    spdlog::debug("is working");

    auto const nfds = epoll_wait(fds[EV_FD_EPOLL], &event, 1, -1);
    if (nfds == -1) [[unlikely]]
      return EXIT_FAILURE;

    for (int i = 0; i < std::min(1, nfds); ++i) {
      if (event.data.fd != fds[EV_FD_LISTEN]) [[unlikely]]
        continue;

      uint32_t cmds;
      if (auto const r = read(fds[EV_FD_LISTEN], &cmds, sizeof(cmds)); r != 4)
          [[unlikely]]
        return EXIT_FAILURE;

      for (cmds -= std::exchange(last_cmds, cmds); cmds; --cmds) {
        auto cmd = qcmd.pop();
        for (; !cmd; cmd = qcmd.pop())
          std::this_thread::yield();

        if (uint32_t processed_cmds = 1;
            4 != write(fds[EV_FD_NOTIFY_REQ], &processed_cmds,
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

} // namespace cfq
