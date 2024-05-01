#pragma once

namespace ublk {

template <typename> class ISubmitter;

template <typename R, typename... Args> class ISubmitter<R(Args...)> {
public:
  ISubmitter() = default;
  virtual ~ISubmitter() = default;

  ISubmitter(ISubmitter const &) = delete;
  ISubmitter &operator=(ISubmitter const &) = delete;

  ISubmitter(ISubmitter &&) noexcept = delete;
  ISubmitter &operator=(ISubmitter &&) noexcept = delete;

  virtual R submit(Args...) = 0;
};

template <typename R, typename... Args> class ISubmitter<R(Args...) noexcept> {
public:
  ISubmitter() = default;
  virtual ~ISubmitter() = default;

  ISubmitter(ISubmitter const &) = delete;
  ISubmitter &operator=(ISubmitter const &) = delete;

  ISubmitter(ISubmitter &&) noexcept = delete;
  ISubmitter &operator=(ISubmitter &&) noexcept = delete;

  virtual R submit(Args...) noexcept = 0;
};

} // namespace ublk
