#pragma once

#include <memory>

#include <linux/ublkdrv/genl.h>

#include <netlink/genl/ctrl.h>
#include <netlink/genl/genl.h>
#include <netlink/msg.h>
#include <netlink/socket.h>

namespace ublk::genl {

inline auto sock_alloc() noexcept {
  return std::unique_ptr<nl_sock, decltype(&::nl_socket_free)>{
      ::nl_socket_alloc(),
      ::nl_socket_free,
  };
}

inline void sock_close(nl_sock &sock) { ::nl_close(&sock); }

inline auto sock_open() noexcept {
  auto sock = sock_alloc();
  auto new_deleter = [d = sock.get_deleter()](auto *p) {
    ::nl_close(p);
    d(p);
  };
  using T = std::unique_ptr<nl_sock, decltype(new_deleter)>;
  if (!::genl_connect(sock.get()))
    return T{sock.release(), new_deleter};
  else
    sock.reset();
  return T{nullptr, new_deleter};
}

inline auto msg_alloc() noexcept {
  return std::unique_ptr<nl_msg, decltype(&::nlmsg_free)>{
      ::nlmsg_alloc(),
      ::nlmsg_free,
  };
}

inline auto resolve(nl_sock &sock) noexcept {
  return ::genl_ctrl_resolve(&sock, UBLKDRV_GENL_NAME);
}

inline auto auto_send(nl_sock &sock, nl_msg &msg) noexcept {
  return ::nl_send_auto_complete(&sock, &msg);
}

} // namespace ublk::genl
