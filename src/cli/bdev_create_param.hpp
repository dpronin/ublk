#pragma once

#include <filesystem>
#include <string>

namespace cfq::cli {

struct bdev_create_param {
  std::string bdev_suffix;
  std::filesystem::path target;
};

} // namespace cfq::cli
