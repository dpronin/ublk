#pragma once

#include <cassert>

#include <memory>
#include <utility>

#include "flush_handler_interface.hpp"
#include "flush_query.hpp"
#include "flush_req.hpp"
#include "handler_interface.hpp"

namespace ublk {

class CmdFlushHandler
    : public IHandler<int(std::shared_ptr<flush_req>) noexcept> {
public:
  explicit CmdFlushHandler(std::shared_ptr<IFlushHandler> flusher)
      : flusher_(std::move(flusher)) {
    assert(flusher_);
  }
  ~CmdFlushHandler() override = default;

  CmdFlushHandler(CmdFlushHandler const &) = default;
  CmdFlushHandler &operator=(CmdFlushHandler const &) = default;

  CmdFlushHandler(CmdFlushHandler &&) = default;
  CmdFlushHandler &operator=(CmdFlushHandler &&) = default;

  int handle(std::shared_ptr<flush_req> req) noexcept override {
    assert(req);
    auto completer = [req = std::move(req)](flush_query const &fq
                                            [[maybe_unused]]) {
      if (fq.err())
        req->set_err(fq.err());
    };
    return flusher_->submit(flush_query::create(completer));
  }

private:
  std::shared_ptr<IFlushHandler> flusher_;
};

} // namespace ublk
