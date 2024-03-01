#pragma once

#include <cstdint>

#include <filesystem>
#include <string>
#include <variant>
#include <vector>

namespace ublk {

struct target_null_cfg {};

struct target_inmem_cfg {};

struct target_default_cfg {
  uint64_t cache_len_sectors;
  bool cache_write_through;
  std::filesystem::path path;
};

struct target_raid0_cfg {
  uint64_t strip_len_sectors;
  uint64_t cache_len_strips;
  bool cache_write_through;
  std::vector<std::filesystem::path> paths;
};

struct target_raid1_cfg {
  uint64_t read_len_sectors_per_path;
  uint64_t cache_len_sectors;
  bool cache_write_through;
  std::vector<std::filesystem::path> paths;
};

struct target_raid4_cfg {
  uint64_t strip_len_sectors;
  uint64_t cache_len_stripes;
  bool cache_write_through;
  std::vector<std::filesystem::path> data_paths;
  std::filesystem::path parity_path;
};

struct target_raid5_cfg {
  uint64_t strip_len_sectors;
  uint64_t cache_len_stripes;
  bool cache_write_through;
  std::vector<std::filesystem::path> paths;
};

struct target_raid10_cfg {
  uint64_t strip_len_sectors;
  uint64_t cache_len_strips;
  bool cache_write_through;
  std::vector<target_raid1_cfg> raid1s;
};

struct target_raid40_cfg {
  uint64_t strip_len_sectors;
  uint64_t cache_len_strips;
  bool cache_write_through;
  std::vector<target_raid4_cfg> raid4s;
};

struct target_raid50_cfg {
  uint64_t strip_len_sectors;
  uint64_t cache_len_strips;
  bool cache_write_through;
  std::vector<target_raid5_cfg> raid5s;
};

struct target_create_param {
  std::string name;
  uint64_t capacity_sectors;
  std::variant<target_null_cfg, target_inmem_cfg, target_default_cfg,
               target_raid0_cfg, target_raid1_cfg, target_raid4_cfg,
               target_raid5_cfg, target_raid10_cfg, target_raid40_cfg,
               target_raid50_cfg>
      target_cfg;
};

} // namespace ublk
