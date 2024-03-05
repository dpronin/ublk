#pragma once

#include <cassert>
#include <cstdint>

#include <boost/dynamic_bitset/dynamic_bitset.hpp>

namespace ublk {

template <typename Allocator> class bitset_rw_semaphore final {
private:
  using state_t = boost::dynamic_bitset<uint64_t, Allocator>;

  void extend(uint64_t nr, state_t &lock_state) {
    if (nr > lock_state.size())
      lock_state.resize(nr);
  }

public:
  explicit bitset_rw_semaphore(uint64_t preallocate_size = 0,
                               Allocator const alloc = {})
      : rlock_state_(preallocate_size, 0, alloc),
        wlock_state_(preallocate_size, 0, alloc) {}
  ~bitset_rw_semaphore() = default;

  bitset_rw_semaphore(bitset_rw_semaphore const &) = delete;
  bitset_rw_semaphore &operator=(bitset_rw_semaphore const &) = delete;

  bitset_rw_semaphore(bitset_rw_semaphore &&) = delete;
  bitset_rw_semaphore &operator=(bitset_rw_semaphore &&) = delete;

  void extend(uint64_t nr) noexcept {
    for (auto *p_lock_state : {&rlock_state_, &wlock_state_})
      extend(nr, *p_lock_state);
  }

  bool try_read_lock(uint64_t pos) noexcept {
    assert(pos < rlock_state_.size());
    return !rlock_state_.test_set(pos);
  }

  void read_unlock(uint64_t pos) noexcept {
    assert(pos < rlock_state_.size());
    assert(rlock_state_[pos]);
    rlock_state_.reset(pos);
  }

  bool try_write_lock(uint64_t pos) noexcept {
    assert(pos < wlock_state_.size());
    return !wlock_state_.test_set(pos);
  }

  void write_unlock(uint64_t pos) noexcept {
    assert(pos < wlock_state_.size());
    assert(wlock_state_[pos]);
    wlock_state_.reset(pos);
  }

private:
  state_t rlock_state_, wlock_state_;
};

} // namespace ublk
