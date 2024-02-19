#include <benchmark/benchmark.h>

#include <concepts>
#include <span>

#include "bench_mem_utility.hpp"

#include "algo.hpp"

namespace ublk::bench {

namespace detail {

template <typename T>
  requires std::integral<T> || is_byte<T>
void fill_bench_with(std::invocable<std::span<T>, T> auto *f,
                     benchmark::State &state) {
  auto const sz = static_cast<size_t>(state.range(0));
  auto dst = make_buffer(sz);
  for (auto _ : state) {
    f(
        std::span{
            reinterpret_cast<T *>(dst.get()),
            reinterpret_cast<T *>(dst.get() + sz),
        },
        T{0});
    benchmark::ClobberMemory();
  }
}

} // namespace detail

template <typename T>
  requires std::integral<T> || is_byte<T>
void fill_stl_bench(std::span<T> range, T value) {
  ublk::algo::detail::fill_stl(range, value);
}

template <typename T>
  requires std::integral<T> || is_byte<T>
void fill_eve_bench(std::span<T> range, T value) {
  ublk::algo::detail::fill_eve(range, value);
}

} // namespace ublk::bench

namespace {

template <typename U> void fill_stl_bench(benchmark::State &state) {
  ublk::bench::detail::fill_bench_with<U>(&ublk::bench::fill_stl_bench<U>,
                                          state);
}

template <typename U> void fill_eve_bench(benchmark::State &state) {
  ublk::bench::detail::fill_bench_with<U>(&ublk::bench::fill_eve_bench<U>,
                                          state);
}

} // namespace

BENCHMARK_TEMPLATE(fill_stl_bench, std::byte)
    ->Arg(1u << 25)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(fill_eve_bench, std::byte)
    ->Arg(1u << 25)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(fill_stl_bench, uint16_t)
    ->Arg(1u << 25)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(fill_eve_bench, uint16_t)
    ->Arg(1u << 25)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(fill_stl_bench, uint32_t)
    ->Arg(1u << 25)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(fill_eve_bench, uint32_t)
    ->Arg(1u << 25)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(fill_stl_bench, uint64_t)
    ->Arg(1u << 25)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_TEMPLATE(fill_eve_bench, uint64_t)
    ->Arg(1u << 25)
    ->Unit(benchmark::kMicrosecond);

BENCHMARK_MAIN();
