#pragma once

#include <cassert>

#include "ublk_req_handler_interface.hpp"

namespace ublk {

template <int eVal> class ReqOpErrHandler : public IUblkReqHandler {

public:
  ReqOpErrHandler() = default;
  ~ReqOpErrHandler() override = default;

  ReqOpErrHandler(ReqOpErrHandler const &) = default;
  ReqOpErrHandler &operator=(ReqOpErrHandler const &) = default;

  ReqOpErrHandler(ReqOpErrHandler &&) = default;
  ReqOpErrHandler &operator=(ReqOpErrHandler &&) = default;

  int handle(std::shared_ptr<ublk_req> req [[maybe_unused]]) noexcept override {
    assert(req);
    req->set_err(eVal);
    return 0;
  }
};

} // namespace ublk
