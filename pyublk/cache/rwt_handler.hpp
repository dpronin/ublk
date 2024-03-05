#pragma once

#include <cstdint>

#include <memory>
#include <utility>
#include <vector>

#include "write_query.hpp"

#include "rwi_handler.hpp"

namespace ublk::cache {

class RWTHandler : public RWIHandler {
public:
  using RWIHandler::RWIHandler;
  ~RWTHandler() override = default;

  RWTHandler(RWTHandler const &) = delete;
  RWTHandler &operator=(RWTHandler const &) = delete;

  RWTHandler(RWTHandler &&) = delete;
  RWTHandler &operator=(RWTHandler &&) = delete;

  int submit(std::shared_ptr<write_query> wq) noexcept override;

private:
  int process(std::shared_ptr<write_query> wq) noexcept;

  std::vector<std::pair<uint64_t, std::shared_ptr<write_query>>> wqs_pending_;
};

} // namespace ublk::cache
