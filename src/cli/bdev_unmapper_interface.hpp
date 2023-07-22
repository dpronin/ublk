#pragma once

#include "bdev_unmap_param.hpp"
#include "handler_interface.hpp"

namespace cfq::cli {
using IBdevUnmapper = IHandler<void(bdev_unmap_param const &)>;
} // namespace cfq::cli
