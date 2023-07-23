#pragma once

namespace ublk {

template <typename T> class ICreator {
public:
  ICreator() = default;
  virtual ~ICreator() = default;

  ICreator(ICreator const &) = default;
  ICreator &operator=(ICreator const &) = default;

  ICreator(ICreator &&) = default;
  ICreator &operator=(ICreator &&) = default;

  virtual void create(T param) = 0;
};

} // namespace ublk
