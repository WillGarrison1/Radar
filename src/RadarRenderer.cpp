#include "RadarRenderer.hpp"

#include <stdexcept>
#include <algorithm>
#include <cmath>

RadarRenderer::RadarRenderer() : shouldQuit(false), renderer(nullptr), window(nullptr)
{
    if (!SDL_WasInit(SDL_INIT_VIDEO))
    {
        bool success = SDL_Init(SDL_INIT_VIDEO);
        if (!success)
        {
            throw std::runtime_error("Failed to initialize SDL!");
        }
    }
    if (!SDL_CreateWindowAndRenderer("Radar", 800, 600, 0, &window, &renderer))
    {
        throw std::runtime_error("Failed to create window and renderer!");
    }
}

RadarRenderer::~RadarRenderer()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

void DrawCircle(SDL_Renderer *renderer, SDL_FPoint center, float radius, SDL_FColor color)
{
    constexpr int resolution = 35; // twenty points along the circle
    SDL_Vertex vertices[resolution + 1];
    int indices[resolution * 3];

    for (int i = 0; i < resolution; i++)
    {
        float angle = 2 * std::numbers::pi * i / resolution; // angle in radians
        vertices[i].tex_coord = {0, 0};
        vertices[i].color = color;
        vertices[i].position = {std::cos(angle) * radius + center.x,
                                std::sin(angle) * radius + center.y};
        indices[i * 3] = resolution;
        indices[i * 3 + 1] = i;
        indices[i * 3 + 2] = (i + 1) % resolution;
    }
    vertices[resolution].color = color;
    vertices[resolution].tex_coord = {0, 0};
    vertices[resolution].position = center;

    SDL_RenderGeometry(renderer, nullptr, vertices, resolution + 1, indices, resolution * 3);
}

void RadarRenderer::Update(VolumeScan &scan)
{
    SDL_SetRenderDrawColorFloat(renderer, 0.15, 0.15, 0.15, 1.0);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColorFloat(renderer, 0.05, 0.05, 0.05, 1.0);

    SDL_FPoint circleCenter{400, 300};
    float circleRadius = 250;

    DrawCircle(renderer, circleCenter, circleRadius, {0, 0, 0, 0});

    // radius of circle is 50km
    SDL_SetRenderDrawColorFloat(renderer, 1, 0, 0, 1);

    auto lowestEl = std::ranges::min_element(scan.radials, {},
                                             [](const auto &pair)
                                             { return pair.first; });

    if (lowestEl != scan.radials.end())
    {
        for (auto &radialAzi : lowestEl->second)
        {
            auto &radial = radialAzi.second;
            for (int i = 0; i < radial.gates.size(); i++)
            {
                auto &gate = radial.gates[i];
                if (!std::isnan(gate.reflectivity) && gate.reflectivity > 15)
                {
                    constexpr float deg2rad = std::numbers::pi / 180.0f;

                    float az = (90 - radial.trueAzimuth) * deg2rad; // convert from north orientation to math conventions
                    float el = radial.trueElevation * deg2rad;

                    float dist = i * radial.gateSize + radial.firstGate;
                    float horizDist = std::cos(el) * dist;

                    if (horizDist > 50000)
                        continue;

                    float x = std::cos(az) * horizDist;
                    float y = std::sin(az) * horizDist;

                    x *= circleRadius / 50000.0f;
                    y *= circleRadius / 50000.0f;
                    x += circleCenter.x;
                    y += circleCenter.y;

                    SDL_RenderPoint(renderer, x, y);
                }
            }
        }
    }

    SDL_RenderPresent(renderer);

    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_EVENT_QUIT)
        {
            shouldQuit = true;
        }
    }
}