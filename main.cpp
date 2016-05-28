#include "papi.h"

#include <iostream>
#include <array>
#include <vector>
#include <chrono>
#include <algorithm>

#include <boost/preprocessor/repetition/repeat.hpp>

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

#define junk(n) std::vector<int> v ## n(200, n); std::cout << std::accumulate(std::begin(v ## n), std::end(v ## n), 1) << std::endl;

//#define junk(n) std::cout << "foobar" << # n << std::endl;

void mispred(const std::array<unsigned char, 255>& v)
{
  #define DECL(z, n, text) if (__builtin_expect(v[n] > n, 1)) { junk(n) }
  BOOST_PP_REPEAT(255, DECL, bla)
}

void pred(const std::array<unsigned char, 255>& v)
{
  #define DECL2(z, n, text) if (__builtin_expect(v[n] > n, 0)) { junk(n) }
  BOOST_PP_REPEAT(255, DECL2, bla)
}

using icache_profiler = papi_wrapper<PAPI_L1_ICM, PAPI_L2_ICM, PAPI_TLB_IM>;

struct F
{
    template <typename F>
    static void Call(F f, icache_profiler& p, int training)
    {
        using clock = std::chrono::high_resolution_clock;

        for (int i = 0; i < training; ++i)
            f();

        p.start();
        auto start = clock::now();
        f();
        auto end = clock::now();
        p.stop();

        std::cout << "time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << "ns" << std::endl;

        const auto& counters = p.get_counters();
        std::cout << "L1 ICM " << counters[0] << " - L2 ICM " << counters[1] << " - TLB IM " << counters[2] << std::endl;
    }
};

int main(int argc, char **argv)
{
    int training = 0;

    if (argc == 2)
        training = std::atoi(argv[1]);

    std::array<unsigned char, 255> v{0};

    icache_profiler p;
    p.start();
    p.stop();

    F::Call([&v]() { mispred(v); }, p, training);
    F::Call([&v]() { pred(v); }, p, training);

    return 0;
}
