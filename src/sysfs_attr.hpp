#pragma once

#include <filesystem>
#include <fstream>

namespace cfq {

template <typename Target, typename... Args>
Target get_sysfs_attr(std::filesystem::path const &attr_path,
                      Args &&...modifiers) {
  Target v;
  std::ifstream f{attr_path};
  ((f >> std::forward<Args>(modifiers)), ...);
  f >> v;
  return v;
}

} // namespace cfq
