#include "target.hpp"

#include <cassert>
#include <cerrno>

#include <span>
#include <utility>

#include "mm/mem_types.hpp"

#include "algo.hpp"

namespace ublk::inmem {

Target::Target(mm::uptrwd<std::byte[]> mem, uint64_t sz) noexcept
    : mem_(std::move(mem)), mem_sz_(sz) {
  assert(mem_);
}

int Target::process(std::shared_ptr<read_query> rq) noexcept {
  assert(rq);

  if (mem_sz_ < rq->offset() + rq->buf().size()) [[unlikely]]
    return EINVAL;

  auto const from =
      std::span<std::byte const>{mem_.get() + rq->offset(), rq->buf().size()};
  auto const to = rq->buf();

  algo::copy(from, to);

  return 0;
}

int Target::process(std::shared_ptr<write_query> wq) noexcept {
  assert(wq);

  if (mem_sz_ < wq->offset() + wq->buf().size()) [[unlikely]]
    return EINVAL;

  auto const from = wq->buf();
  auto const to = std::span{mem_.get() + wq->offset(), wq->buf().size()};

  algo::copy(from, to);

  return 0;
}

} // namespace ublk::inmem
