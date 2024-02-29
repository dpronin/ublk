#pragma once

#include <cstdint>

#include <memory>
#include <vector>

#include "rw_handler_interface.hpp"
#include "read_query.hpp"
#include "write_query.hpp"

namespace ublk::raid0 {

class Target final {
public:
  explicit Target(uint64_t strip_sz,
                  std::vector<std::shared_ptr<IRWHandler>> hs);

  int process(std::shared_ptr<read_query> rq) noexcept;
  int process(std::shared_ptr<write_query> wq) noexcept;

private:
  int read(uint64_t strip_id_from, uint64_t strip_offset_from,
           std::shared_ptr<read_query> rq) noexcept;
  int write(uint64_t strip_id_from, uint64_t strip_offset_from,
            std::shared_ptr<write_query> wq) noexcept;

  uint64_t strip_sz_;
  std::vector<std::shared_ptr<IRWHandler>> hs_;
};

} // namespace ublk::raid0
