#include "papi.h"

#include <iostream>
#include <array>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#include <iomanip>

#include <boost/preprocessor/repetition/repeat.hpp>

#define junk_small(n)  std::exit(n); // 10 bytes of junk on x86-64
#define junk_medium(n) std::cout << "foobar" << # n << std::endl; // ~100 bytes on x86-64
#define junk_big(n)    std::vector<int> v ## n(200, n); std::cout << "foobar" << # n << std::accumulate(std::begin(v ## n), std::end(v ## n), n) << std::endl; // ~250 bytes on x86-64

#define junk junk_big

void wrong_hint(const std::array<unsigned char, 255>& v)
{
    #define CONDITION(z, n, text) if (__builtin_expect(v[n] > n, 1)) { junk(n) }
    BOOST_PP_REPEAT(255, CONDITION, bla)
    #undef CONDITION
}

void no_hint(const std::array<unsigned char, 255>& v)
{
    #define CONDITION(z, n, text) if (v[n] > n) { junk(n) }
    BOOST_PP_REPEAT(255, CONDITION, bla)
    #undef CONDITION
}

void correct_hint(const std::array<unsigned char, 255>& v)
{
    #define CONDITION(z, n, text) if (__builtin_expect(v[n] > n, 0)) { junk(n) }
    BOOST_PP_REPEAT(255, CONDITION, bla)
    #undef CONDITION
}

using icache_profiler = papi_wrapper<PAPI_L1_ICM, PAPI_L2_ICM, PAPI_TLB_IM>;
using branch_profiler = papi_wrapper<PAPI_BR_CN, PAPI_BR_PRC, PAPI_BR_MSP>;
using instr_profiler = papi_wrapper<PAPI_TOT_CYC, PAPI_TOT_INS>;

template <typename F, typename _ProfilerT>
std::chrono::nanoseconds profile(F f, _ProfilerT& p, int training)
{
    using clock = std::chrono::high_resolution_clock;

    for (int i = 0; i < training; ++i)
        f();

    p.start();
    auto start = clock::now();
    f();
    auto end = clock::now();
    p.stop();

    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
}

template <typename F>
std::chrono::nanoseconds profile(F f, int training)
{
    using clock = std::chrono::high_resolution_clock;

    for (int i = 0; i < training; ++i)
        f();

    auto start = clock::now();
    f();
    auto end = clock::now();

    return std::chrono::duration_cast<std::chrono::nanoseconds>(end - start);
}

template <typename _ProfilerT>
void print_papi(const char* descr, _ProfilerT& p, std::chrono::nanoseconds duration)
{
    std::cout << std::left << std::setw(18) << descr << "time: " << duration.count() << "ns --- ";

    for (int i = 0; i < _ProfilerT::events_count; ++i)
        std::cout << _ProfilerT::get_event_name(i) << ": " << p.get_counter(i) << "  ";
    std::cout << std::endl;
}

template <typename _ProfilerT>
void profile_papi(const std::array<unsigned char, 255>& v, int training)
{
    _ProfilerT p;

    // this hack because on the first start() call, it seems that libpapi is doing some init and measurements are biased
    p.start(); p.stop();

    auto duration = profile([&v]() { wrong_hint(v); }, p, training);
    print_papi("wrong hint", p, duration);

    duration = profile([&v]() { no_hint(v); }, p, training);
    print_papi("no hint", p, duration);

    duration = profile([&v]() { correct_hint(v); }, p, training);
    print_papi("correct hint", p, duration);
};

int usage(char** argv)
{
    std::cerr << "usage: " << argv[0] << " training [cache|branch|instr]" << std::endl;
    return 1;
}

int main(int argc, char **argv)
{
    if (argc != 2 && argc != 3)
        return usage(argv);

    int training = std::atoi(argv[1]);
    std::string hwd_counters = argc == 3 ? argv[2] : "";

    std::array<unsigned char, 255> v{};

    if (hwd_counters.empty())
    {
        auto dur_wh = profile([&v]() { wrong_hint(v); }, training);
        auto dur_nh = profile([&v]() { no_hint(v); }, training);
        auto dur_ch = profile([&v]() { correct_hint(v); }, training);

        std::cout << dur_wh.count() << ";" << dur_nh.count() << ";" << dur_ch.count() << std::endl;
    }
    else if (hwd_counters == "cache")
        profile_papi<instr_profiler>(v, training);
    else if (hwd_counters == "branch")
        profile_papi<instr_profiler>(v, training);
    else if (hwd_counters == "instr")
        profile_papi<instr_profiler>(v, training);

    return 0;
}
