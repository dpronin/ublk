#pragma once

#include <cstdint>

#include <filesystem>
#include <string>

namespace cfq::cli {

struct bdev_create_param {
  std::string bdev_suffix;
  uint64_t capacity_sectors;
  std::filesystem::path target;
  bool read_only;
};

} // namespace cfq::cli
