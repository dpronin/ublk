#pragma once

#include "read_handler_interface.hpp"
#include "read_query.hpp"
#include "write_query.hpp"
#include "wrq_submitter_interface.hpp"

namespace ublk {

class IRWHandler : public IReadHandler, public IWRQSubmitter {
public:
  IRWHandler() = default;
  ~IRWHandler() override = default;

  IRWHandler(IRWHandler const &) = delete;
  IRWHandler &operator=(IRWHandler const &) = delete;

  IRWHandler(IRWHandler &&) = delete;
  IRWHandler &operator=(IRWHandler &&) = delete;

  int submit(std::shared_ptr<read_query> rq) noexcept override = 0;
  int submit(std::shared_ptr<write_query> wq) noexcept override = 0;
};

} // namespace ublk
