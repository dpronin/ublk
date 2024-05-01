#pragma once

#include <boost/asio/io_context.hpp>

#include "slave_param.hpp"

namespace ublk::slave {
void run(boost::asio::io_context &io_ctx, slave_param const &param);
} // namespace ublk::slave
