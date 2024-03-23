#pragma once

#include <cstdint>

#include <memory>
#include <ranges>
#include <vector>

#include "rw_handler_interface.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

namespace ublk::raid0 {

class Target final {
public:
  explicit Target(uint64_t strip_sz,
                  std::vector<std::shared_ptr<IRWHandler>> hs);

  explicit Target(uint64_t strip_sz, std::ranges::input_range auto &&hs)
      : Target(strip_sz, {std::ranges::begin(hs), std::ranges::end(hs)}) {}

  ~Target() noexcept;

  Target(Target const &) = delete;
  Target &operator=(Target const &) = delete;

  Target(Target &&) noexcept;
  Target &operator=(Target &&) noexcept;

  std::string state() const;

  int process(std::shared_ptr<read_query> rq) noexcept;
  int process(std::shared_ptr<write_query> wq) noexcept;

private:
  class impl;
  std::unique_ptr<impl> pimpl_;
};

} // namespace ublk::raid0
