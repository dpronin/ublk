#pragma once

#include <memory>
#include <string>

#include "ublk_req_handler_interface.hpp"

namespace ublk::slave {

struct slave_param {
  std::string bdev_suffix;
  std::shared_ptr<IUblkReqHandler> handler;
};

} // namespace ublk::slave
