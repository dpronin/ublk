#pragma once

#include "bdev_create_param.hpp"
#include "handler_interface.hpp"

namespace cfq::cli {
using IBdevCreator = IHandler<void(bdev_create_param const &)>;
} // namespace cfq::cli
