#pragma once

namespace ublk {

template <typename T> class IDestroyer {
public:
  IDestroyer() = default;
  virtual ~IDestroyer() = default;

  IDestroyer(IDestroyer const &) = default;
  IDestroyer &operator=(IDestroyer const &) = default;

  IDestroyer(IDestroyer &&) = default;
  IDestroyer &operator=(IDestroyer &&) = default;

  virtual void destroy(T param) = 0;
};

} // namespace ublk
