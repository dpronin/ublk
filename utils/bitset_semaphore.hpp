#pragma once

#include <cassert>
#include <cstdint>

#include <boost/dynamic_bitset/dynamic_bitset.hpp>

namespace ublk {

template <typename Allocator> class bitset_semaphore final {
private:
  using state_t = boost::dynamic_bitset<uint64_t, Allocator>;

public:
  explicit bitset_semaphore(uint64_t preallocate_size = 0,
                            Allocator const alloc = {})
      : lock_state_(preallocate_size, 0, alloc) {}
  ~bitset_semaphore() = default;

  bitset_semaphore(bitset_semaphore const &) = delete;
  bitset_semaphore &operator=(bitset_semaphore const &) = delete;

  bitset_semaphore(bitset_semaphore &&) = delete;
  bitset_semaphore &operator=(bitset_semaphore &&) = delete;

  void extend(uint64_t nr) noexcept {
    if (nr > lock_state_.size())
      lock_state_.resize(nr);
  }

  bool try_lock(uint64_t pos) noexcept {
    assert(pos < lock_state_.size());
    return !lock_state_.test_set(pos);
  }

  void unlock(uint64_t pos) noexcept {
    assert(pos < lock_state_.size());
    assert(lock_state_[pos]);
    lock_state_.reset(pos);
  }

private:
  state_t lock_state_;
};

} // namespace ublk
