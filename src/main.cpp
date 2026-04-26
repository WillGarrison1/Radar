#include <iostream>
#include <format>
#include <thread>

#include "Radar/RadarRenderer.hpp"
#include "Radar/StormProcessor.hpp"
#include "Globe/IcosphereRenderer.hpp"
#include "Globe/IcoflatRenderer.hpp"

constexpr float TARGET_FPS = 60;
constexpr float TARGET_FRAMETIME = 1 / TARGET_FPS;

enum class RenderType
{
    Radar,
    Icosphere,
    Icoflat
};

int main(int argc, char **argv)
{
    constexpr RenderType t = RenderType::Icoflat;
    Renderer *renderer;

    switch (t)
    {
    case RenderType::Radar:
        renderer = new RadarRenderer();
        break;
    case RenderType::Icosphere:
        renderer = new IcosphereRenderer();
        break;
    case RenderType::Icoflat:
        renderer = new IcoflatRenderer();
        break;
    default:
        std::cerr << "Unknown Renderer" << std::endl;
        return 1;
    }

    float delta = 1e-5;
    while (!renderer->ShouldQuit())
    {
        auto start = std::chrono::steady_clock::now();
        renderer->Update(delta);
        auto end = std::chrono::steady_clock::now();
        delta = std::chrono::duration_cast<std::chrono::microseconds>(end - start).count() / 1000000.0f;
        std::this_thread::sleep_for(std::chrono::microseconds{static_cast<uint64_t>((TARGET_FRAMETIME - delta) * 1000000)});
        std::cout << "\r" << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << "ms";
    }

    delete renderer;

    return 0;
}