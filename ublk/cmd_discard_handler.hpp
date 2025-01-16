#pragma once

#include <memory>
#include <utility>

#include <gsl/assert>

#include <linux/ublkdrv/cmd.h>

#include "discard_handler_interface.hpp"
#include "discard_query.hpp"
#include "discard_req.hpp"
#include "handler_interface.hpp"

namespace ublk {

class CmdDiscardHandler
    : public IHandler<int(std::shared_ptr<discard_req>) noexcept> {
public:
  explicit CmdDiscardHandler(std::shared_ptr<IDiscardHandler> discarder)
      : discarder_(std::move(discarder)) {
    Ensures(discarder_);
  }
  ~CmdDiscardHandler() override = default;

  CmdDiscardHandler(CmdDiscardHandler const &) = default;
  CmdDiscardHandler &operator=(CmdDiscardHandler const &) = default;

  CmdDiscardHandler(CmdDiscardHandler &&) = default;
  CmdDiscardHandler &operator=(CmdDiscardHandler &&) = default;

  int handle(std::shared_ptr<discard_req> req) noexcept override {
    Expects(req);
    auto *p_req = req.get();
    auto completer = [req = std::move(req)](discard_query const &dq) {
      if (dq.err())
        req->set_err(dq.err());
    };
    return discarder_->submit(discard_query::create(
        ublkdrv_cmd_discard_get_offset(&p_req->cmd()),
        ublkdrv_cmd_discard_get_sz(&p_req->cmd()), completer));
  }

private:
  std::shared_ptr<IDiscardHandler> discarder_;
};

} // namespace ublk
