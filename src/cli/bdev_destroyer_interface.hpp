#pragma once

#include "bdev_destroy_param.hpp"
#include "handler_interface.hpp"

namespace cfq::cli {
using IBdevDestroyer = IHandler<void(bdev_destroy_param const &)>;
} // namespace cfq::cli
