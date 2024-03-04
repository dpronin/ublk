#include <benchmark/benchmark.h>

#include <concepts>
#include <span>

#include "utils/algo.hpp"

#include "bench_mem_utility.hpp"

namespace ublk::bench {

namespace detail {

template <typename T>
  requires std::integral<T> || is_byte<T>
void copy_bench_with(std::invocable<std::span<T const>, std::span<T>> auto *f,
                     benchmark::State &state) {
  auto const sz = static_cast<size_t>(state.range(0));
  auto dst = make_buffer(sz);
  auto src = make_buffer(sz);
  for (auto _ : state) {
    f(
        std::span{
            reinterpret_cast<T const *>(src.get()),
            reinterpret_cast<T const *>(src.get() + sz),
        },
        std::span{
            reinterpret_cast<T *>(dst.get()),
            reinterpret_cast<T *>(dst.get() + sz),
        });
    benchmark::ClobberMemory();
  }
}

} // namespace detail

template <typename T>
  requires std::integral<T> || is_byte<T>
void copy_stl_bench(std::span<T const> from, std::span<T> to) {
  ublk::algo::detail::copy_stl(from, to);
}

template <typename T>
  requires std::integral<T> || is_byte<T>
void copy_eve_bench(std::span<T const> from, std::span<T> to) {
  ublk::algo::detail::copy_eve(from, to);
}

} // namespace ublk::bench

namespace {

template <typename U> void copy_stl_bench(benchmark::State &state) {
  ublk::bench::detail::copy_bench_with<U>(&ublk::bench::copy_stl_bench<U>,
                                          state);
}

template <typename U> void copy_eve_bench(benchmark::State &state) {
  ublk::bench::detail::copy_bench_with<U>(&ublk::bench::copy_eve_bench<U>,
                                          state);
}

} // namespace

BENCHMARK_TEMPLATE(copy_stl_bench, std::byte)
    ->Arg(1u << 25)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(copy_eve_bench, std::byte)
    ->Arg(1u << 25)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(copy_stl_bench, uint16_t)
    ->Arg(1u << 25)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(copy_eve_bench, uint16_t)
    ->Arg(1u << 25)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(copy_stl_bench, uint32_t)
    ->Arg(1u << 25)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(copy_eve_bench, uint32_t)
    ->Arg(1u << 25)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(copy_stl_bench, uint64_t)
    ->Arg(1u << 25)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(copy_eve_bench, uint64_t)
    ->Arg(1u << 25)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
