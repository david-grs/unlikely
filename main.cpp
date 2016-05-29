#include "papi.h"

#include <iostream>
#include <array>
#include <vector>
#include <chrono>
#include <algorithm>
#include <cstdlib>
#include <iomanip>
#include <cassert>

#include <boost/preprocessor/repetition/repeat.hpp>

#define junk_small(n)  std::exit(n); // 10 bytes of junk on x86-64
#define junk_medium(n) std::cout << "foobar" << # n << std::endl; // ~100 bytes on x86-64
#define junk_big(n)    std::vector<int> v ## n(200, n); std::cout << "foobar" << # n << std::accumulate(std::begin(v ## n), std::end(v ## n), n) << std::endl; // ~250 bytes on x86-64

#define junk junk_small

void not_taken_wrong_hint(const std::array<unsigned char, 255>& v)
{
    #define CONDITION(z, n, text) if (__builtin_expect(v[n] > n, 1)) { junk(n) }
    BOOST_PP_REPEAT(255, CONDITION, bla)
    #undef CONDITION
}

void not_taken_no_hint(const std::array<unsigned char, 255>& v)
{
    #define CONDITION(z, n, text) if (v[n] > n) { junk(n) }
    BOOST_PP_REPEAT(255, CONDITION, bla)
    #undef CONDITION
}

void not_taken_correct_hint(const std::array<unsigned char, 255>& v)
{
    #define CONDITION(z, n, text) if (__builtin_expect(v[n] > n, 0)) { junk(n) }
    BOOST_PP_REPEAT(255, CONDITION, bla)
    #undef CONDITION
}

#define one_instr(n)  ++i;
#define few_instrs(n) i = (i + 1) * n; i = (int)std::round(i / std::sqrt(1 + v[n])); i = std::max(i, 1000);

#define instr few_instrs

int taken_wrong_hint(const std::array<unsigned char, 255>& v)
{
    int i = 1;
    #define CONDITION(z, n, text) if (__builtin_expect(v[n] <= n, 0)) { instr(n) }
    BOOST_PP_REPEAT(255, CONDITION, bla)
    #undef CONDITION
    return i;
}

int taken_no_hint(const std::array<unsigned char, 255>& v)
{
    int i = 1;
    #define CONDITION(z, n, text) if (v[n] <= n) { instr(n) }
    BOOST_PP_REPEAT(255, CONDITION, bla)
    #undef CONDITION
    return i;
}

int taken_correct_hint(const std::array<unsigned char, 255>& v)
{
    int i = 1;
    #define CONDITION(z, n, text) if (__builtin_expect(v[n] <= n, 1)) { instr(n) }
    BOOST_PP_REPEAT(255, CONDITION, bla)
    #undef CONDITION
    return i;
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

template <typename _ProfilerT>
void print_papi(const char* descr, _ProfilerT& p, std::chrono::nanoseconds duration)
{
    std::cout << std::left << std::setw(18) << descr << "time: " << duration.count() << "ns --- ";

    for (int i = 0; i < _ProfilerT::events_count; ++i)
        std::cout << _ProfilerT::get_event_name(i) << ": " << p.get_counter(i) << "  ";
    std::cout << std::endl;
}

template <typename _ProfilerT>
void profile_papi(const std::array<unsigned char, 255>& v, bool taken, int training)
{
    _ProfilerT p;

    // this hack because on the first start() call, it seems that libpapi is doing some initialization and I got
    // weird results / jitter for the first measurement without that line
    p.start(); p.stop();

    if (taken)
    {
        auto duration = profile([&v]() { taken_wrong_hint(v); }, p, training);
        print_papi("wrong hint", p, duration);

        duration = profile([&v]() { taken_no_hint(v); }, p, training);
        print_papi("no hint", p, duration);

        duration = profile([&v]() { taken_correct_hint(v); }, p, training);
        print_papi("correct hint", p, duration);
    }
    else
    {
        auto duration = profile([&v]() { not_taken_wrong_hint(v); }, p, training);
        print_papi("wrong hint", p, duration);

        duration = profile([&v]() { not_taken_no_hint(v); }, p, training);
        print_papi("no hint", p, duration);

        duration = profile([&v]() { not_taken_correct_hint(v); }, p, training);
        print_papi("correct hint", p, duration);
    }
};

int usage(char** argv)
{
    std::cerr << "usage: " << argv[0] << " taken|not-taken training [cache|branch|instr]" << std::endl;
    return 1;
}

int main(int argc, char **argv)
{
    if ((argc != 3 && argc != 4) || (argv[1] != std::string("taken") && argv[1] != std::string("not-taken")))
        return usage(argv);

    const bool taken = argv[1] == std::string("taken");
    const int training = std::atoi(argv[2]);
    const std::string hwd_counters = argc == 4 ? argv[3] : "";

    std::array<unsigned char, 255> v{};

    if (hwd_counters.empty())
    {
        std::chrono::nanoseconds dur_wh, dur_nh, dur_ch;

        if (taken)
        {
            dur_wh = profile([&v]() { taken_wrong_hint(v); }, training);
            dur_nh = profile([&v]() { taken_no_hint(v); }, training);
            dur_ch = profile([&v]() { taken_correct_hint(v); }, training);
        }
        else
        {
            dur_wh = profile([&v]() { not_taken_wrong_hint(v); }, training);
            dur_nh = profile([&v]() { not_taken_no_hint(v); }, training);
            dur_ch = profile([&v]() { not_taken_correct_hint(v); }, training);
        }

        std::cout << dur_wh.count() << ";" << dur_nh.count() << ";" << dur_ch.count() << std::endl;
    }
    else
    {
        using cache_profiler  = papi_wrapper<PAPI_L1_ICM, PAPI_L2_ICM, PAPI_TLB_IM>;
        using branch_profiler = papi_wrapper<PAPI_BR_CN, PAPI_BR_PRC, PAPI_BR_MSP>;
        using instr_profiler  = papi_wrapper<PAPI_TOT_CYC, PAPI_TOT_INS>;

        if (hwd_counters == "cache")
            profile_papi<cache_profiler>(v, taken, training);
        else if (hwd_counters == "branch")
            profile_papi<branch_profiler>(v, taken, training);
        else if (hwd_counters == "instr")
            profile_papi<instr_profiler>(v, taken, training);
        else
            assert(false);
    }

    return 0;
}
