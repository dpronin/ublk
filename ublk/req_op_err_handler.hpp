#pragma once

#include <gsl/assert>

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

  int handle(std::shared_ptr<req> req [[maybe_unused]]) noexcept override {
    Expects(req);
    req->set_err(eVal);
    return 0;
  }
};

} // namespace ublk
