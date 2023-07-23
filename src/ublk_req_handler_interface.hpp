#pragma once

#include <memory>

#include "handler_interface.hpp"
#include "ublk_req.hpp"

namespace ublk {
using IUblkReqHandler = IHandler<int(std::shared_ptr<ublk_req>) noexcept>;
} // namespace ublk
