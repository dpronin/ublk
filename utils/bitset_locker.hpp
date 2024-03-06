#pragma once

#include <cassert>
#include <cstddef>

#include <boost/dynamic_bitset/dynamic_bitset.hpp>

namespace ublk {

template <typename Block, typename Allocator> class bitset_locker final {
private:
  using state_t = boost::dynamic_bitset<Block, Allocator>;

public:
  explicit bitset_locker(size_t preallocate_size = 0, Allocator const alloc = {})
      : lk_state_(preallocate_size, 0, alloc) {}
  ~bitset_locker() = default;

  bitset_locker(bitset_locker const &) = delete;
  bitset_locker &operator=(bitset_locker const &) = delete;

  bitset_locker(bitset_locker &&) = delete;
  bitset_locker &operator=(bitset_locker &&) = delete;

  void extend(size_t nr) noexcept {
    if (nr > lk_state_.size())
      lk_state_.resize(nr);
  }

  bool is_locked(size_t pos) const noexcept {
    assert(pos < lk_state_.size());
    return lk_state_[pos];
  }

  bool try_lock(size_t pos) noexcept {
    assert(pos < lk_state_.size());
    return !lk_state_.test_set(pos);
  }

  void unlock(size_t pos) noexcept {
    assert(pos < lk_state_.size());
    assert(lk_state_[pos]);
    lk_state_.reset(pos);
  }

private:
  state_t lk_state_;
};

} // namespace ublk
