#pragma once

#include <cstdint>

#include "bitset_semaphore.hpp"

namespace ublk {

template <typename Allocator> class bitset_rw_semaphore final {
public:
  explicit bitset_rw_semaphore(uint64_t preallocate_size = 0,
                               Allocator const alloc = {})
      : rstate_(preallocate_size, alloc), wstate_(preallocate_size, alloc) {}
  ~bitset_rw_semaphore() = default;

  bitset_rw_semaphore(bitset_rw_semaphore const &) = delete;
  bitset_rw_semaphore &operator=(bitset_rw_semaphore const &) = delete;

  bitset_rw_semaphore(bitset_rw_semaphore &&) = delete;
  bitset_rw_semaphore &operator=(bitset_rw_semaphore &&) = delete;

  void extend(uint64_t nr) noexcept {
    for (auto *p_state : {&rstate_, &wstate_})
      p_state->extend(nr);
  }

  bool try_read_lock(uint64_t pos) noexcept { return rstate_.try_lock(pos); }
  void read_unlock(uint64_t pos) noexcept { rstate_.unlock(pos); }

  bool try_write_lock(uint64_t pos) noexcept { return wstate_.try_lock(pos); }
  void write_unlock(uint64_t pos) noexcept { wstate_.unlock(pos); }

private:
  bitset_semaphore<Allocator> rstate_, wstate_;
};

} // namespace ublk
