#include <benchmark/benchmark.h>

#include <concepts>
#include <format>
#include <ranges>
#include <span>
#include <stdexcept>

#include "utils/math.hpp"

#include "bench_mem_utility.hpp"

namespace ublk::bench {

namespace detail {

template <typename T>
  requires std::integral<T> || is_byte<T>
void xor_bench_with(std::invocable<std::span<T const>, std::span<T>> auto *f,
                    benchmark::State &state) {
  auto const src_sz = static_cast<size_t>(state.range(0));
  auto const dst_sz = static_cast<size_t>(state.range(1));

  if (src_sz % dst_sz)
    throw std::invalid_argument(std::format(
        "src_sz must be a multiple of dst_sz, actual src_sz {}, dst_sz {}",
        src_sz, dst_sz));

  auto src = make_buffer(src_sz);
  auto dst = make_buffer(dst_sz);
  for (auto _ : state) {
    f(
        std::span{
            reinterpret_cast<T const *>(src.get()),
            reinterpret_cast<T const *>(src.get() + src_sz),
        },
        std::span{
            reinterpret_cast<T *>(dst.get()),
            reinterpret_cast<T *>(dst.get() + dst_sz),
        });
    benchmark::ClobberMemory();
  }
}

} // namespace detail

template <typename T>
  requires std::integral<T> || is_byte<T>
void xor_stl_bench(std::span<T const> in, std::span<T> inout) {
  for (auto ch_in : in | std::views::chunk(inout.size()))
    ublk::math::detail::xor_to_stl(
        std::span<T const>{
            std::ranges::begin(ch_in),
            std::ranges::end(ch_in),
        },
        inout);
}

template <typename T>
  requires std::integral<T> || is_byte<T>
void xor_eve_bench(std::span<T const> in, std::span<T> inout) {
  for (auto ch_in : in | std::views::chunk(inout.size()))
    ublk::math::detail::xor_to_eve(
        std::span<T const>{
            std::ranges::begin(ch_in),
            std::ranges::end(ch_in),
        },
        inout);
}

} // namespace ublk::bench

namespace {

template <typename U> void xor_stl_bench(benchmark::State &state) {
  ublk::bench::detail::xor_bench_with<U>(&ublk::bench::xor_stl_bench<U>, state);
}

template <typename U> void xor_eve_bench(benchmark::State &state) {
  ublk::bench::detail::xor_bench_with<U>(&ublk::bench::xor_eve_bench<U>, state);
}

} // namespace

BENCHMARK_TEMPLATE(xor_stl_bench, std::byte)
    ->Args({1u << 25, 1u << 17})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(xor_eve_bench, std::byte)
    ->Args({1u << 25, 1u << 17})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(xor_stl_bench, uint16_t)
    ->Args({1u << 25, 1u << 17})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(xor_eve_bench, uint16_t)
    ->Args({1u << 25, 1u << 17})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(xor_stl_bench, uint32_t)
    ->Args({1u << 25, 1u << 17})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(xor_eve_bench, uint32_t)
    ->Args({1u << 25, 1u << 17})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(xor_stl_bench, uint64_t)
    ->Args({1u << 25, 1u << 17})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(xor_eve_bench, uint64_t)
    ->Args({1u << 25, 1u << 17})
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
