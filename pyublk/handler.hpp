#pragma once

#include <linux/ublkdrv/cmd.h>

#include <filesystem>
#include <memory>
#include <string>
#include <unordered_map>

#include <boost/asio/io_context.hpp>

#include "handler_interface.hpp"
#include "qublkcmd.hpp"

namespace ublk {

using evpaths_t = std::unordered_map<std::string, std::filesystem::path>;

void async_start(evpaths_t const &evpaths, boost::asio::io_context &io_ctx,
                 std::unique_ptr<qublkcmd_t> qcmd,
                 std::unique_ptr<IHandler<int(ublkdrv_cmd) noexcept>> handler);

} // namespace ublk
