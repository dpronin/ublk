#pragma once

#include "handler_interface.hpp"
#include "ublk_req.hpp"

namespace ublk {

template <int eVal>
class ReqOpErrHandler
    : public IHandler<int(std::shared_ptr<ublk_req>) noexcept> {

public:
  ReqOpErrHandler() = default;
  ~ReqOpErrHandler() override = default;

  ReqOpErrHandler(ReqOpErrHandler const &) = default;
  ReqOpErrHandler &operator=(ReqOpErrHandler const &) = default;

  ReqOpErrHandler(ReqOpErrHandler &&) = default;
  ReqOpErrHandler &operator=(ReqOpErrHandler &&) = default;

  int handle(std::shared_ptr<ublk_req> req [[maybe_unused]]) noexcept override {
    req->set_err(eVal);
    return 0;
  }
};

} // namespace ublk
