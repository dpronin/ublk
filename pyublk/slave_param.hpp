#pragma once

#include <memory>
#include <span>
#include <string>

#include <linux/ublkdrv/celld.h>
#include <linux/ublkdrv/cmd.h>
#include <linux/ublkdrv/cmd_ack.h>

#include "factory_unique_interface.hpp"
#include "handler_interface.hpp"

namespace ublk::slave {

struct slave_param {
  std::string bdev_suffix;
  std::shared_ptr<
      IFactoryUnique<rvwrap<IHandler<int(ublkdrv_cmd const &) noexcept>>(
          std::span<ublkdrv_celld const>, std::span<std::byte>,
          std::shared_ptr<IHandler<int(ublkdrv_cmd_ack) noexcept>>)>>
      hfactory;
};

} // namespace ublk::slave
