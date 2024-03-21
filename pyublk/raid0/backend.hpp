#pragma once

#include <cstdint>

#include <concepts>
#include <memory>
#include <utility>
#include <vector>

#include "mm/mem_types.hpp"

#include "rw_handler_interface.hpp"

namespace ublk::raid0 {

class backend final {
public:
  explicit backend(uint64_t strip_sz,
                   std::vector<std::shared_ptr<IRWHandler>> hs);

  int process(std::shared_ptr<read_query> rq) noexcept;
  int process(std::shared_ptr<write_query> wq) noexcept;

private:
  template <typename T>
    requires std::same_as<T, write_query> || std::same_as<T, read_query>
  int do_op(std::shared_ptr<T> query) noexcept;

  struct static_cfg {
    uint64_t strip_sz;
  };

  mm::uptrwd<static_cfg const> static_cfg_;
  std::vector<std::shared_ptr<IRWHandler>> hs_;
};

} // namespace ublk::raid0
