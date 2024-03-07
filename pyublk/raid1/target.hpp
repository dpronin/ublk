#pragma once

#include <memory>
#include <vector>

#include "rw_handler_interface.hpp"

namespace ublk::raid1 {

class Target final {
public:
  explicit Target(uint64_t read_len_bytes_per_handler,
                  std::vector<std::shared_ptr<IRWHandler>> hs) noexcept;

  int process(std::shared_ptr<read_query> rq) noexcept;
  int process(std::shared_ptr<write_query> wq) noexcept;

private:
  uint64_t read_len_bytes_per_handler_;

  uint32_t next_hid_;
  std::vector<std::shared_ptr<IRWHandler>> hs_;
};

} // namespace ublk::raid1
