#pragma once

#include <linux/ublk/cmd.h>

#include <filesystem>
#include <string>
#include <unordered_map>

#include "handler_interface.hpp"
#include "qublkcmd.hpp"

namespace cfq {

using evpaths_t = std::unordered_map<std::string, std::filesystem::path>;

int handler(qublkcmd_t &qcmd, evpaths_t const &evpaths,
            IHandler<int(ublk_cmd) noexcept> &handler);

} // namespace cfq
