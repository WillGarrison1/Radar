#include <iostream>
#include <format>
#include <thread>

#include "RadarRenderer.hpp"
#include "StormProcessor.hpp"

constexpr float TARGET_FPS = 60;
constexpr float TARGET_FRAMETIME = 1 / TARGET_FPS;

int main(int argc, char **argv)
{
    RadarRenderer renderer;
    float delta = 1e-5;
    while (!renderer.ShouldQuit())
    {
        auto start = std::chrono::steady_clock::now();
        renderer.Update(delta);
        auto end = std::chrono::steady_clock::now();
        delta = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000.0f;
        std::this_thread::sleep_for(std::chrono::microseconds{static_cast<uint64_t>((TARGET_FRAMETIME - delta) * 1000000)});
        std::cout << "\r" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms";
    }

    return 0;
}