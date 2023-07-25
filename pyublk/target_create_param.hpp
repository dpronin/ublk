#pragma once

#include <cstdint>

#include <filesystem>
#include <string>

namespace ublk {

struct target_create_param {
  std::string name;
  uint64_t capacity_sectors;
  std::filesystem::path path;
};

} // namespace ublk
