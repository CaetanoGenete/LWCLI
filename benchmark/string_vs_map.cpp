#include "benchmark/benchmark.h"

#include <string>
#include <random>
#include <functional>
#include <algorithm>
#include <unordered_set>

template<class RandomEngine>
static std::vector<std::string> random_arguments(unsigned int size, RandomEngine& eng)
{
    constexpr unsigned int ALPHABET_SIZE = 'Z' - 'A' + 1;
    constexpr unsigned int MIN_STR_SIZE = 10;
    constexpr unsigned int MAX_STR_SIZE = 20;

    std::uniform_int_distribution dist;
    auto rand = std::bind(dist, std::ref(eng));

    std::vector<std::string> result(size);
    for (auto& arg : result) {
        const unsigned int str_size = MIN_STR_SIZE + rand() % (MAX_STR_SIZE - MIN_STR_SIZE + 1);
        arg.reserve(str_size + 2);

        arg += "--";
        for (unsigned int i = 0; i < str_size; ++i)
            arg += (rand() % 2 ? 'A' : 'a') + rand() % ALPHABET_SIZE;
    }
    return result;
}

static void BM_vector_search(benchmark::State& state)
{
    auto size = static_cast<unsigned int>(state.range(0));
    std::default_random_engine eng(size);

    auto args = random_arguments(size, eng);
    auto search_order = args;
    std::ranges::shuffle(search_order, eng);

    for (auto _ : state) {
        decltype(args)::iterator loc;
        for (const auto& arg : search_order)
            loc = std::ranges::find(args, arg);
        benchmark::DoNotOptimize(loc);
    }
}
BENCHMARK(BM_vector_search)->RangeMultiplier(2)->Range(16, 1024);

static void BM_map_seach(benchmark::State& state)
{
    auto size = static_cast<unsigned int>(state.range(0));
    std::default_random_engine eng(size);

    auto search_order = random_arguments(size, eng);
    std::unordered_set<std::string> args;
    args.insert(search_order.begin(), search_order.end());

    for (auto _ : state) {
        auto has = false;
        for (const auto& arg : search_order)
            has ^= args.contains(arg);
        benchmark::DoNotOptimize(has);
    }
}
BENCHMARK(BM_map_seach)->RangeMultiplier(2)->Range(16, 1024);