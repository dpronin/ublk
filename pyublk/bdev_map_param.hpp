#pragma once

#include <string>

namespace ublk {

struct bdev_map_param {
  std::string bdev_suffix;
  std::string target_name;
  bool read_only;
};

} // namespace ublk
