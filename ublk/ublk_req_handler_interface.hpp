#pragma once

#include <memory>

#include "handler_interface.hpp"
#include "req.hpp"

namespace ublk {
using IUblkReqHandler = IHandler<int(std::shared_ptr<req>) noexcept>;
} // namespace ublk
