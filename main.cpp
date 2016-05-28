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
using branch_profiler = papi_wrapper<PAPI_BR_CN, PAPI_BR_PRC, PAPI_BR_MSP>;

struct F
{
    template <typename F, typename _ProfilerT>
    static void Call(F f, _ProfilerT& p, int training)
    {
        p.start();
        p.stop();

        using clock = std::chrono::high_resolution_clock;

        for (int i = 0; i < training; ++i)
            f();

        p.start();
        auto start = clock::now();
        f();
        auto end = clock::now();
        p.stop();

        std::cout << "time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << "ns" << std::endl;

        for (int i = 0; i < _ProfilerT::events_count; ++i)
            std::cout << _ProfilerT::get_event_name(i) << ": " << p.get_counter(i) << "  ";
        std::cout << std::endl;
    }
};

int usage(char** argv)
{
    std::cerr << "usage: " << argv[0] << " <icache|branch> [training]" << std::endl;
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 3)
        return usage(argv);

    int training = argc == 3 ? std::atoi(argv[2]) : 0;
    std::array<unsigned char, 255> v{0};

    if (argv[1] == std::string("icache"))
    {
        icache_profiler p;
        F::Call([&v]() { mispred(v); }, p, training);
        F::Call([&v]() { pred(v); }, p, training);
    }
    else if (argv[1] == std::string("branch"))
    {
        branch_profiler p;
        F::Call([&v]() { mispred(v); }, p, training);
        F::Call([&v]() { pred(v); }, p, training);
    }
    else
    {
        return usage(argv);
    }

    return 0;
}
