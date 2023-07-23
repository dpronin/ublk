#pragma once

namespace cfq {

template <typename T> class IUnmapper {
public:
  IUnmapper() = default;
  virtual ~IUnmapper() = default;

  IUnmapper(IUnmapper const &) = default;
  IUnmapper &operator=(IUnmapper const &) = default;

  IUnmapper(IUnmapper &&) = default;
  IUnmapper &operator=(IUnmapper &&) = default;

  virtual void unmap(T param) = 0;
};

} // namespace cfq
