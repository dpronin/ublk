#pragma once

#include <memory>

#include "handler_interface.hpp"
#include "read_handler_interface.hpp"
#include "read_req.hpp"

namespace ublk {

class CmdReadHandler
    : public IHandler<int(std::shared_ptr<read_req>) noexcept> {
public:
  explicit CmdReadHandler(std::shared_ptr<IReadHandler> reader);
  ~CmdReadHandler() override = default;

  CmdReadHandler(CmdReadHandler const &) = default;
  CmdReadHandler &operator=(CmdReadHandler const &) = default;

  CmdReadHandler(CmdReadHandler &&) = default;
  CmdReadHandler &operator=(CmdReadHandler &&) = default;

  int handle(std::shared_ptr<read_req> req) noexcept override;

private:
  std::shared_ptr<IReadHandler> reader_;
};

} // namespace ublk
