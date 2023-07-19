#include "master.hpp"

#include <cstdint>
#include <cstdlib>
#include <cstring>

#include <sys/wait.h>
#include <unistd.h>

#include <linux/ublk/cellc.h>
#include <linux/ublk/celld.h>
#include <linux/ublk/cfqcb.h>
#include <linux/ublk/genl.h>

#include <stdexcept>
#include <utility>

#include <spdlog/spdlog.h>

#include "genl.hpp"

#include "slave.hpp"

using namespace cfq;

Master::~Master() {
  /* While there are children_ we would wait for them all to exit */
  while (!children_.empty()) {
    auto const child = *children_.begin();
    if (kill(child, SIGTERM) < 0)
      kill(child, SIGKILL);
    int wstatus{0};
    auto const childw = waitpid(child, &wstatus, 0);
    if (WIFEXITED(wstatus))
      spdlog::info("childw {} exited", childw);
    children_.erase(childw);
  }
}

void Master::create(cli::bdev_create_param const &param) {
  auto nl_sock = genl::sock_open();
  auto msg = genl::msg_alloc();

  genlmsg_put(msg.get(), 0, 0, genl::resolve(*nl_sock), 0, 0,
              UBLK_GENL_BDEV_CMD_CREATE, 1);
  nla_put_string(msg.get(), UBLK_GENL_BDEV_ATTR_NAME_SUFFIX,
                 param.bdev_suffix.c_str());

  genl::auto_send(*nl_sock, *msg);

  nl_sock.reset();

  if (auto const child_pid = fork(); 0 == child_pid) {
    children_.clear();
    slave::run(param);
  } else if (child_pid > 0) {
    /* We're in a parent's body, remember a new child's PID */
    children_.insert(child_pid);
  } else {
    throw std::runtime_error(fmt::format(
        "{}: failed to create child, reason: {}", getpid(), strerror(errno)));
  }
}

void Master::destroy(cli::bdev_destroy_param const &param) {
  auto nl_sock = genl::sock_open();
  auto msg = genl::msg_alloc();

  genlmsg_put(msg.get(), 0, 0, genl::resolve(*nl_sock), 0, 0,
              UBLK_GENL_BDEV_CMD_DESTROY, 1);
  nla_put_string(msg.get(), UBLK_GENL_BDEV_ATTR_NAME_SUFFIX,
                 param.bdev_suffix.data());

  genl::auto_send(*nl_sock, *msg);
}
