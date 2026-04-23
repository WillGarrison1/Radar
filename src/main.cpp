#include <iostream>
#include <format>
#include <thread>

#include "RadarRenderer.hpp"
#include "StormProcessor.hpp"

int main(int argc, char **argv)
{
    RadarRenderer renderer;
    while (!renderer.ShouldQuit())
    {
        auto start = std::chrono::steady_clock::now();
        renderer.Update();
        auto end = std::chrono::steady_clock::now();
        std::cout << "\r" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms";
    }

    return 0;
}