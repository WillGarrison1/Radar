#pragma once

#include <chrono>
#include <iostream>

template <typename duration>
class Benchmark
{

public:
    Benchmark() : start(std::chrono::steady_clock::now())
    {
    }
    ~Benchmark()
    {
        std::cout << "Time: " << std::chrono::duration_cast<duration>(std::chrono::steady_clock::now() - start).count() << "ms" << std::endl;
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> start;
};

#define BENCHMARK_MS() \
    Benchmark<std::chrono::milliseconds> __timer_{}