#pragma once

#include <cerrno>

#include "req_op_err_handler.hpp"

namespace cfq {
using ReqHandlerNotSupp = ReqOpErrHandler<ENOTSUP>;
} // namespace cfq
