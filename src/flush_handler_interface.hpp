#pragma once

namespace cfq {

class IFlushHandler {
public:
  IFlushHandler() = default;
  virtual ~IFlushHandler() = default;

  IFlushHandler(IFlushHandler const &) = default;
  IFlushHandler &operator=(IFlushHandler const &) = default;

  IFlushHandler(IFlushHandler &&) = default;
  IFlushHandler &operator=(IFlushHandler &&) = default;

  virtual int handle() noexcept = 0;
};

} // namespace cfq
