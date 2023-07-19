#pragma once

#include <cerrno>

#include "cmd_op_err_handler.hpp"

namespace cfq {
using CmdHandlerNotSupp = CmdOpErrHandler<ENOTSUP>;
} // namespace cfq
