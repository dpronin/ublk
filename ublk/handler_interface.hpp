#pragma once

namespace ublk {

template <typename> class IHandler;

template <typename R, typename... Args> class IHandler<R(Args...)> {
public:
  IHandler() = default;
  virtual ~IHandler() = default;

  IHandler(IHandler const &) = default;
  IHandler &operator=(IHandler const &) = default;

  IHandler(IHandler &&) noexcept = default;
  IHandler &operator=(IHandler &&) noexcept = default;

  virtual R handle(Args...) = 0;
};

template <typename R, typename... Args> class IHandler<R(Args...) noexcept> {
public:
  IHandler() = default;
  virtual ~IHandler() = default;

  IHandler(IHandler const &) = default;
  IHandler &operator=(IHandler const &) = default;

  IHandler(IHandler &&) noexcept = default;
  IHandler &operator=(IHandler &&) noexcept = default;

  virtual R handle(Args...) noexcept = 0;
};

} // namespace ublk
