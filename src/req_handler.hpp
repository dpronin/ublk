#pragma once

#include <array>
#include <map>
#include <memory>
#include <utility>

#include <linux/ublk/cmd.h>

#include "ublk_req.hpp"
#include "ublk_req_handler_interface.hpp"

namespace ublk {

class ReqHandler : public IUblkReqHandler {
public:
  explicit ReqHandler(
      std::map<ublk_cmd_op, std::shared_ptr<IUblkReqHandler>> const &maphs);
  ~ReqHandler() override = default;

  ReqHandler(ReqHandler const &) = default;
  ReqHandler &operator=(ReqHandler const &) = default;

  ReqHandler(ReqHandler &&) = default;
  ReqHandler &operator=(ReqHandler &&) = default;

  int handle(std::shared_ptr<ublk_req> req) noexcept override;

private:
  std::array<std::shared_ptr<IUblkReqHandler>, UBLK_CMD_OP_MAX + 2> hs_;
};

} // namespace ublk
