#pragma once

namespace cfq {

template <typename T> class IMapper {
public:
  IMapper() = default;
  virtual ~IMapper() = default;

  IMapper(IMapper const &) = default;
  IMapper &operator=(IMapper const &) = default;

  IMapper(IMapper &&) = default;
  IMapper &operator=(IMapper &&) = default;

  virtual void map(T param) = 0;
};

} // namespace cfq
