#pragma once

#include <memory>

#include "rvwrap.hpp"

namespace ublk {

template <typename> class IFactoryUnique;

template <typename R, typename... Args> class IFactoryUnique<R(Args...)> {
public:
  IFactoryUnique() = default;
  virtual ~IFactoryUnique() = default;

  IFactoryUnique(IFactoryUnique const &) = default;
  IFactoryUnique &operator=(IFactoryUnique const &) = default;

  IFactoryUnique(IFactoryUnique &&) noexcept = default;
  IFactoryUnique &operator=(IFactoryUnique &&) noexcept = default;

  virtual std::unique_ptr<R> create_unique(Args...) = 0;
};

template <typename R, typename... Args>
class IFactoryUnique<R(Args...) noexcept> {
public:
  IFactoryUnique() = default;
  virtual ~IFactoryUnique() = default;

  IFactoryUnique(IFactoryUnique const &) = default;
  IFactoryUnique &operator=(IFactoryUnique const &) = default;

  IFactoryUnique(IFactoryUnique &&) noexcept = default;
  IFactoryUnique &operator=(IFactoryUnique &&) noexcept = default;

  virtual std::unique_ptr<R> create_unique(Args...) noexcept = 0;
};

template <typename R, typename... Args>
class IFactoryUnique<rvwrap<R>(Args...)> {
public:
  IFactoryUnique() = default;
  virtual ~IFactoryUnique() = default;

  IFactoryUnique(IFactoryUnique const &) = default;
  IFactoryUnique &operator=(IFactoryUnique const &) = default;

  IFactoryUnique(IFactoryUnique &&) noexcept = default;
  IFactoryUnique &operator=(IFactoryUnique &&) noexcept = default;

  virtual std::unique_ptr<R> create_unique(Args...) = 0;
};

template <typename R, typename... Args>
class IFactoryUnique<rvwrap<R>(Args...) noexcept> {
public:
  IFactoryUnique() = default;
  virtual ~IFactoryUnique() = default;

  IFactoryUnique(IFactoryUnique const &) = default;
  IFactoryUnique &operator=(IFactoryUnique const &) = default;

  IFactoryUnique(IFactoryUnique &&) noexcept = default;
  IFactoryUnique &operator=(IFactoryUnique &&) noexcept = default;

  virtual std::unique_ptr<R> create_unique(Args...) noexcept = 0;
};

} // namespace ublk
