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

int main(int argc, char **argv)
{
    int training = 0;

    if (argc == 2)
        training = std::atoi(argv[1]);

    std::array<unsigned char, 255> v{0};

    using clock = std::chrono::high_resolution_clock;

    {
        for (int i = 0; i < training; ++i)
            mispred(v);

        auto start = clock::now();
        mispred(v);
        auto end = clock::now();

        std::cout << "mispred: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << std::endl;
    }

    {
        for (int i = 0; i < training; ++i)
            pred(v);

        auto start = clock::now();
        pred(v);
        auto end = clock::now();

        std::cout << "pred: " << std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count() << std::endl;
    }

    return 0;
}
