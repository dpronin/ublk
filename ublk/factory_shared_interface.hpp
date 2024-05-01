#pragma once

#include <memory>

namespace ublk {

template <typename> class IFactoryShared;

template <typename R, typename... Args> class IFactoryShared<R(Args...)> {
public:
  IFactoryShared() = default;
  virtual ~IFactoryShared() = default;

  IFactoryShared(IFactoryShared const &) = default;
  IFactoryShared &operator=(IFactoryShared const &) = default;

  IFactoryShared(IFactoryShared &&) noexcept = default;
  IFactoryShared &operator=(IFactoryShared &&) noexcept = default;

  virtual std::shared_ptr<R> create_shared(Args...) = 0;
};

template <typename R, typename... Args>
class IFactoryShared<R(Args...) noexcept> {
public:
  IFactoryShared() = default;
  virtual ~IFactoryShared() = default;

  IFactoryShared(IFactoryShared const &) = default;
  IFactoryShared &operator=(IFactoryShared const &) = default;

  IFactoryShared(IFactoryShared &&) noexcept = default;
  IFactoryShared &operator=(IFactoryShared &&) noexcept = default;

  virtual std::shared_ptr<R> create_shared(Args...) noexcept = 0;
};

} // namespace ublk
