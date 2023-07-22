#pragma once

#include <future>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "cli/bdev_map_param.hpp"
#include "cli/bdev_unmap_param.hpp"
#include "cli/target_create_param.hpp"
#include "cli/target_destroy_param.hpp"

#include "target.hpp"

namespace cfq {

class Master {
public:
  Master() = default;
  ~Master();

  Master(Master const &) = delete;
  Master &operator=(Master const &) = delete;

  Master(Master &&) = delete;
  Master &operator=(Master &&) = delete;

  void map(cli::bdev_map_param const &param);
  void unmap(cli::bdev_unmap_param const &param);
  void create(cli::target_create_param const &param);
  void destroy(cli::target_destroy_param const &param);

private:
  std::unordered_map<pid_t, std::future<void>> children_;
  std::unordered_map<std::string, std::shared_ptr<Target>> targets_;
};

} // namespace cfq
