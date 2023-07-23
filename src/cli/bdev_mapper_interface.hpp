#pragma once

#include "bdev_map_param.hpp"
#include "handler_interface.hpp"

namespace ublk::cli {
using IBdevMapper = IHandler<void(bdev_map_param const &)>;
} // namespace ublk::cli
