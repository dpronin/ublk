#pragma once

#include <cstddef>
#include <cstdint>

#include "mem_types.hpp"
#include "read_query.hpp"
#include "write_query.hpp"

namespace ublk::inmem {

class Target final {
public:
  explicit Target(uptrwd<std::byte[]> mem, uint64_t sz) noexcept;

  int process(std::shared_ptr<read_query> rq) noexcept;
  int process(std::shared_ptr<write_query> wq) noexcept;

private:
  uptrwd<std::byte[]> mem_;
  uint64_t mem_sz_;
};

} // namespace ublk::inmem
