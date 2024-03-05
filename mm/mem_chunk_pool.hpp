#pragma once

#include <cassert>
#include <cstddef>

#include <functional>
#include <stack>

#include "mem_types.hpp"

#include "utils/span.hpp"
#include "utils/utility.hpp"

namespace ublk::mm {

class mem_chunk_pool final {
public:
  explicit mem_chunk_pool(std::function<uptrwd<std::byte[]>()> generator,
                          size_t alignment, size_t chunk_sz) noexcept
      : generator_(std::move(generator)), alignment_(alignment),
        chunk_sz_(chunk_sz) {
    assert(generator_);
    assert(is_power_of_2(alignment_));
  }
  ~mem_chunk_pool() = default;

  mem_chunk_pool(mem_chunk_pool const &) = delete;
  mem_chunk_pool &operator=(mem_chunk_pool const &) = delete;

  mem_chunk_pool(mem_chunk_pool &&) = delete;
  mem_chunk_pool &operator=(mem_chunk_pool &&) = delete;

  size_t chunk_alignment() const noexcept { return alignment_; }
  size_t chunk_sz() const noexcept { return chunk_sz_; }

  template <typename T = std::byte>
  auto chunk_view(uptrwd<std::byte[]> const &mem_chunk) noexcept {
    return to_span_of<T>({mem_chunk.get(), chunk_sz_});
  }

  uptrwd<std::byte[]> get() noexcept {
    auto chunk = uptrwd<std::byte[]>{};
    if (!free_chunks_.empty()) {
      chunk = std::move(free_chunks_.top());
      free_chunks_.pop();
    } else {
      chunk = generator_();
    }
    return {
        chunk.release(),
        [this, d = std::move(chunk.get_deleter())](auto *p) mutable {
          free_chunks_.push({p, std::move(d)});
        },
    };
  }

private:
  std::function<uptrwd<std::byte[]>()> generator_;
  size_t alignment_;
  size_t chunk_sz_;
  std::stack<uptrwd<std::byte[]>> free_chunks_;
};

} // namespace ublk::mm
