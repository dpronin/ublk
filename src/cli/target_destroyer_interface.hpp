#pragma once

#include "handler_interface.hpp"
#include "target_destroy_param.hpp"

namespace ublk::cli {
using ITargetDestroyer = IHandler<void(target_destroy_param const &)>;
} // namespace ublk::cli
