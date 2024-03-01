#pragma once

#include <cassert>
#include <cstddef>
#include <cstdint>

#include <map>
#include <stack>
#include <unordered_map>

#include "mem.hpp"
#include "mem_allocator.hpp"
#include "mem_types.hpp"
#include "utility.hpp"

namespace ublk::mm {

class mem_cached_allocator : public mem_allocator {
private:
  struct free_desc {
    std::stack<uptrwd<std::byte[]>> free_chunks;
  };
  using free_desc_table_t = std::map<size_t, free_desc>;

  struct busy_desc {
    uptrwd<std::byte[]> chunk;
    free_desc_table_t::iterator ftbl_desc_it;
  };
  using free_table_t = std::map<size_t, free_desc_table_t>;
  using busy_table_t = std::unordered_map<uintptr_t, busy_desc>;

public:
  mem_cached_allocator() = default;
  ~mem_cached_allocator() override = default;

  mem_cached_allocator(mem_cached_allocator const &) = delete;
  mem_cached_allocator &operator=(mem_cached_allocator const &) = delete;

  mem_cached_allocator(mem_cached_allocator &&) = delete;
  mem_cached_allocator &operator=(mem_cached_allocator &&) = delete;

  void *allocate_aligned(size_t alignment, size_t sz) noexcept override {
    assert(is_power_of_2(alignment));
    assert(sz);

    alignment = std::max(alignment, alignof(std::max_align_t));
    auto ftbl_it = ftbl_.lower_bound(alignment);
    if (ftbl_it == ftbl_.end())
      ftbl_it = ftbl_.emplace_hint(ftbl_.end(), alignment, free_desc_table_t{});
    alignment = ftbl_it->first;

    sz = std::max(sz, alignment);
    auto &ftbl_desc = ftbl_it->second;
    auto ftbl_desc_it = ftbl_desc.lower_bound(sz);
    if (ftbl_desc_it == ftbl_desc.end())
      ftbl_desc_it = ftbl_desc.emplace_hint(ftbl_desc.end(), sz, free_desc{});
    sz = ftbl_desc_it->first;

    auto &free_chunks = ftbl_desc_it->second.free_chunks;

    if (free_chunks.empty())
      free_chunks.push(get_unique_bytes_generator(alignment, sz)());

    auto uptr{std::move(free_chunks.top())};
    free_chunks.pop();

    auto *p = uptr.get();
    [[maybe_unused]] auto const [it, emplaced] =
        btbl_.emplace(reinterpret_cast<uintptr_t>(p),
                      busy_desc{std::move(uptr), ftbl_desc_it});
    assert(emplaced);

    return p;
  }

  void *allocate(size_t sz) noexcept override {
    return allocate_aligned(alignof(std::max_align_t), sz);
  }

  void free(void *p [[maybe_unused]]) noexcept override {
    auto const it = btbl_.find(reinterpret_cast<uintptr_t>(p));
    assert(it != btbl_.end());
    it->second.ftbl_desc_it->second.free_chunks.push(
        std::move(it->second.chunk));
    btbl_.erase(it);
  }

private:
  free_table_t ftbl_;
  busy_table_t btbl_;
};

inline auto __mem_cached_allocator__ = std::make_shared<mem_cached_allocator>();

} // namespace ublk::mm
