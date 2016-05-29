#include "papi.h"

#include <iostream>
#include <array>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#include <iomanip>

#include <boost/preprocessor/repetition/repeat.hpp>

#define likely(x)      __builtin_expect(!!(x), 1)
#define unlikely(x)    __builtin_expect(!!(x), 0)

#define junk_small(n) std::exit(n); // 10 bytes of junk on x86-64
#define junk_medium(n) std::cout << "foobar" << # n << std::endl; // ~100 bytes on x86-64
#define junk_big(n) std::vector<int> v ## n(200, n); std::cout << std::accumulate(std::begin(v ## n), std::end(v ## n), 1) << std::endl; // ~250 bytes on x86-64

#define junk junk_medium

void wrong_hint(const std::array<unsigned char, 255>& v)
{
    #define CONDITION(z, n, text) if (__builtin_expect(v[n] > n, 1)) { junk(n) }
    BOOST_PP_REPEAT(255, CONDITION, bla)
    #undef CONDITION
}

void correct_hint(const std::array<unsigned char, 255>& v)
{
    #define CONDITION(z, n, text) if (__builtin_expect(v[n] > n, 0)) { junk(n) }
    BOOST_PP_REPEAT(255, CONDITION, bla)
    #undef CONDITION
}

void no_hint(const std::array<unsigned char, 255>& v)
{
    #define CONDITION(z, n, text) if (v[n] > n) { junk(n) }
    BOOST_PP_REPEAT(255, CONDITION, bla)
    #undef CONDITION
}

using icache_profiler = papi_wrapper<PAPI_L1_ICM, PAPI_L2_ICM, PAPI_TLB_IM>;
using branch_profiler = papi_wrapper<PAPI_BR_CN, PAPI_BR_PRC, PAPI_BR_MSP>;
using instr_profiler = papi_wrapper<PAPI_TOT_CYC, PAPI_TOT_INS>;

struct F
{
    template <typename F, typename _ProfilerT>
    static void Call(F f, _ProfilerT& p, int training, const char* descr)
    {
        // this hack because on the first start() call, it seems that libpapi is doing some extra stuff and I saw a
        // little bias in the measurements
        p.start(); p.stop();

        using clock = std::chrono::high_resolution_clock;

        for (int i = 0; i < training; ++i)
            f();

        p.start();
        auto start = clock::now();
        f();
        auto end = clock::now();
        p.stop();

        std::cout << std::left << std::setw(18) << descr << "time: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << "ns --- ";

        for (int i = 0; i < _ProfilerT::events_count; ++i)
            std::cout << _ProfilerT::get_event_name(i) << ": " << p.get_counter(i) << "  ";
        std::cout << std::endl;
    }
};

int usage(char** argv)
{
    std::cerr << "usage: " << argv[0] << " <icache|branch|instr> [training]" << std::endl;
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 3)
        return usage(argv);

    int training = argc == 3 ? std::atoi(argv[2]) : 0;
    std::array<unsigned char, 255> v{};

    auto bench = [&](auto& p)
    {
        F::Call([&v]() { wrong_hint(v);   }, p, training, "wrong hint");
        F::Call([&v]() { correct_hint(v); }, p, training, "correct hint");
        F::Call([&v]() { no_hint(v);      }, p, training, "no hint");
    };

    if (argv[1] == std::string("icache"))
    {
        icache_profiler p;
        bench(p);
    }
    else if (argv[1] == std::string("branch"))
    {
        branch_profiler p;
        bench(p);
    }
    else if (argv[1] == std::string("instr"))
    {
        instr_profiler p;
        bench(p);
    }
    else
    {
        return usage(argv);
    }

    return 0;
}
