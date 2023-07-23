#pragma once

#include "handler_interface.hpp"
#include "target_create_param.hpp"

namespace ublk::cli {
using ITargetCreator = IHandler<void(target_create_param const &)>;
} // namespace ublk::cli
