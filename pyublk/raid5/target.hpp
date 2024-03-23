#pragma once

#include <cstdint>

#include <memory>
#include <ranges>
#include <string>
#include <vector>

#include "rw_handler_interface.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

namespace ublk::raid5 {

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

  bool is_stripe_parity_coherent(uint64_t stripe_id) const noexcept;

  int process(std::shared_ptr<read_query> rq) noexcept;
  int process(std::shared_ptr<write_query> wq) noexcept;

private:
  class impl;
  std::unique_ptr<impl> pimpl_;
};

} // namespace ublk::raid5
