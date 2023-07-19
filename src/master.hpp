#pragma once

#include <unistd.h>

#include <filesystem>
#include <string_view>
#include <unordered_set>

#include "bdev_create_param.hpp"
#include "bdev_destroy_param.hpp"

namespace cfq {

class Master {
public:
  Master() = default;
  ~Master();

  Master(Master const &) = delete;
  Master &operator=(Master const &) = delete;

  Master(Master &&) = delete;
  Master &operator=(Master &&) = delete;

  void create(bdev_create_param const &param);
  void destroy(bdev_destroy_param const &param);

private:
  std::unordered_set<pid_t> children_;
};

} // namespace cfq
