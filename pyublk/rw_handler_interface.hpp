#pragma once

#include "read_handler_interface.hpp"
#include "read_query.hpp"
#include "write_query.hpp"
#include "write_handler_interface.hpp"

namespace ublk {

class IRWHandler : public IReadHandler, public IWriteHandler {
public:
  IRWHandler() = default;
  ~IRWHandler() override = default;

  IRWHandler(IRWHandler const &) = default;
  IRWHandler &operator=(IRWHandler const &) = default;

  IRWHandler(IRWHandler &&) = default;
  IRWHandler &operator=(IRWHandler &&) = default;

  int submit(std::shared_ptr<read_query> rq) noexcept override = 0;
  int submit(std::shared_ptr<write_query> wq) noexcept override = 0;
};

} // namespace ublk
