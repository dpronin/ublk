#pragma once

#include <memory>

#include "handler_interface.hpp"
#include "write_req.hpp"
#include "wrq_submitter_interface.hpp"

namespace ublk {

class CmdWriteHandler
    : public IHandler<int(std::shared_ptr<write_req>) noexcept> {
public:
  explicit CmdWriteHandler(std::shared_ptr<IWRQSubmitter> writer);
  ~CmdWriteHandler() override = default;

  CmdWriteHandler(CmdWriteHandler const &) = default;
  CmdWriteHandler &operator=(CmdWriteHandler const &) = default;

  CmdWriteHandler(CmdWriteHandler &&) = default;
  CmdWriteHandler &operator=(CmdWriteHandler &&) = default;

  int handle(std::shared_ptr<write_req> req) noexcept override;

private:
  std::shared_ptr<IWRQSubmitter> writer_;
};

} // namespace ublk
