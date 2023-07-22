#pragma once

#include "bdev_map_param.hpp"
#include "handler_interface.hpp"

namespace cfq::cli {
using IBdevMapper = IHandler<void(bdev_map_param const &)>;
} // namespace cfq::cli
