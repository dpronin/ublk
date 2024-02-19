#pragma once

#include <cerrno>

#include "req_op_err_handler.hpp"

namespace ublk {
using ReqHandlerNotSupp = ReqOpErrHandler<ENOTSUP>;
} // namespace ublk
