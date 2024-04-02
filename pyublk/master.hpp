#pragma once

#include <cstddef>

#include <future>
#include <list>
#include <string>
#include <unordered_map>
#include <utility>

#include "bdev_map_param.hpp"
#include "bdev_unmap_param.hpp"
#include "target.hpp"
#include "target_create_param.hpp"
#include "target_destroy_param.hpp"
#include "targets_list_param.hpp"

namespace ublk {

class Master {
public:
  static Master &instance() {
    static Master obj;
    return obj;
  }
  ~Master();

  Master(Master const &) = delete;
  Master &operator=(Master const &) = delete;

  Master(Master &&) = delete;
  Master &operator=(Master &&) = delete;

private:
  Master() = default;

public:
  void map(bdev_map_param const &param);
  void unmap(bdev_unmap_param const &param);
  void create(target_create_param const &param);
  void destroy(target_destroy_param const &param);
  std::list<std::pair<std::string, size_t>>
  list(targets_list_param const &param);

private:
  std::unordered_map<pid_t, std::future<void>> children_;
  std::unordered_map<std::string, std::shared_ptr<Target>> targets_;
};

} // namespace ublk
