#pragma once

namespace ublk {

template <typename> class IFactory;

template <typename R, typename... Args> class IFactory<R(Args...)> {
public:
  IFactory() = default;
  virtual ~IFactory() = default;

  IFactory(IFactory const &) = default;
  IFactory &operator=(IFactory const &) = default;

  IFactory(IFactory &&) noexcept = default;
  IFactory &operator=(IFactory &&) noexcept = default;

  virtual R create(Args...) = 0;
};

template <typename R, typename... Args> class IFactory<R(Args...) noexcept> {
public:
  IFactory() = default;
  virtual ~IFactory() = default;

  IFactory(IFactory const &) = default;
  IFactory &operator=(IFactory const &) = default;

  IFactory(IFactory &&) noexcept = default;
  IFactory &operator=(IFactory &&) noexcept = default;

  virtual R create(Args...) noexcept = 0;
};

} // namespace ublk
