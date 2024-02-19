#include "target.hpp"

#include <cassert>
#include <cerrno>

#include <span>
#include <utility>

#include "algo.hpp"
#include "mem_types.hpp"

namespace ublk::inmem {

Target::Target(uptrwd<std::byte[]> mem, uint64_t sz) noexcept
    : mem_(std::move(mem)), mem_sz_(sz) {
  assert(mem_);
}

ssize_t Target::read(std::span<std::byte> buf, __off64_t offset) noexcept {
  if (mem_sz_ < offset + buf.size()) [[unlikely]]
    return -EINVAL;

  auto const from = std::span<std::byte const>{mem_.get() + offset, buf.size()};
  auto const to = buf;

  algo::copy(from, to);

  return buf.size();
}

ssize_t Target::write(std::span<std::byte const> buf,
                      __off64_t offset) noexcept {
  if (mem_sz_ < offset + buf.size()) [[unlikely]]
    return -EINVAL;

  auto const from = buf;
  auto const to = std::span{mem_.get() + offset, buf.size()};

  algo::copy(from, to);

  return buf.size();
}

} // namespace ublk::inmem
