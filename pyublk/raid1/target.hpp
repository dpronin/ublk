#pragma once

#include <cstdint>

#include <memory>
#include <vector>

#include "rw_handler_interface.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

namespace ublk::raid1 {

class Target final {
public:
  explicit Target(uint64_t read_strip_sz,
                  std::vector<std::shared_ptr<IRWHandler>> hs);
  ~Target() noexcept;

  Target(Target const &) = delete;
  Target &operator=(Target const &) = delete;

  Target(Target &&) noexcept;
  Target &operator=(Target &&) noexcept;

  int process(std::shared_ptr<read_query> rq) noexcept;
  int process(std::shared_ptr<write_query> wq) noexcept;

private:
  class impl;
  std::unique_ptr<impl> pimpl_;
};

} // namespace ublk::raid1
