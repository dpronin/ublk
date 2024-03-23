#pragma once

#include <cstdint>

#include <memory>
#include <vector>

#include "mm/mem_types.hpp"

#include "rw_handler_interface.hpp"

#include "read_query.hpp"
#include "write_query.hpp"

namespace ublk::raid1 {

class backend final {
public:
  explicit backend(uint64_t read_strip_sz,
                   std::vector<std::shared_ptr<IRWHandler>> hs) noexcept;
  ~backend() noexcept;

  backend(backend const &) = delete;
  backend &operator=(backend const &) = delete;

  backend(backend &&) noexcept;
  backend &operator=(backend &&) noexcept;

  int process(std::shared_ptr<read_query> rq) noexcept;
  int process(std::shared_ptr<write_query> wq) noexcept;

private:
  struct static_cfg;
  mm::uptrwd<static_cfg const> static_cfg_;

  uint32_t next_hid_;
  std::vector<std::shared_ptr<IRWHandler>> hs_;
};

} // namespace ublk::raid1
