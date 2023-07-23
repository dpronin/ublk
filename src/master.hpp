#pragma once

#include <future>
#include <string>
#include <unordered_map>
#include <unordered_set>

#include "cli/bdev_map_param.hpp"
#include "cli/bdev_unmap_param.hpp"
#include "cli/target_create_param.hpp"
#include "cli/target_destroy_param.hpp"

#include "mapper_interface.hpp"
#include "target.hpp"
#include "unmapper_interface.hpp"

namespace cfq {

class Master : public IMapper<cli::bdev_map_param const &>,
               public IUnmapper<cli::bdev_unmap_param const &> {
public:
  Master() = default;
  ~Master() override;

  Master(Master const &) = delete;
  Master &operator=(Master const &) = delete;

  Master(Master &&) = delete;
  Master &operator=(Master &&) = delete;

  void map(cli::bdev_map_param const &param) override;
  void unmap(cli::bdev_unmap_param const &param) override;
  void create(cli::target_create_param const &param);
  void destroy(cli::target_destroy_param const &param);

private:
  std::unordered_map<pid_t, std::future<void>> children_;
  std::unordered_map<std::string, std::shared_ptr<Target>> targets_;
};

} // namespace cfq
