#pragma once

namespace ublk {

template <typename> class ISubmitter;

template <typename R, typename... Args> class ISubmitter<R(Args...)> {
public:
  ISubmitter() = default;
  virtual ~ISubmitter() = default;

  ISubmitter(ISubmitter const &) = default;
  ISubmitter &operator=(ISubmitter const &) = default;

  ISubmitter(ISubmitter &&) noexcept = default;
  ISubmitter &operator=(ISubmitter &&) noexcept = default;

  virtual R submit(Args...) = 0;
};

template <typename R, typename... Args> class ISubmitter<R(Args...) noexcept> {
public:
  ISubmitter() = default;
  virtual ~ISubmitter() = default;

  ISubmitter(ISubmitter const &) = default;
  ISubmitter &operator=(ISubmitter const &) = default;

  ISubmitter(ISubmitter &&) noexcept = default;
  ISubmitter &operator=(ISubmitter &&) noexcept = default;

  virtual R submit(Args...) noexcept = 0;
};

} // namespace ublk
