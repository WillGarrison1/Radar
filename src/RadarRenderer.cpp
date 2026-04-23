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

SDL_FColor GetColor(float reflectivity)
{
    return {
        std::clamp(reflectivity / 60, 0.0f, 1.0f),
        0,
        1.0f - std::clamp(reflectivity / 60, 0.0f, 1.0f),
        0.75};
}

void RadarRenderer::Update(VolumeScan &scan)
{
    static float circleRadiuspx = 275;
    static float circleRadiusm = 100000;
    static size_t elevationLayer = 0;

    SDL_SetRenderDrawColorFloat(renderer, 0.15, 0.15, 0.15, 1.0);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColorFloat(renderer, 0.05, 0.05, 0.05, 1.0);

    SDL_FPoint circleCenter{400, 300};

    DrawCircle(renderer, circleCenter, circleRadiuspx, {0, 0, 0, 0});

    // radius of circle is 50km
    SDL_SetRenderDrawColorFloat(renderer, 1, 0, 0, 1);

    auto elevationIt = scan.radials.begin();

    std::advance(elevationIt, elevationLayer);

    if (elevationIt != scan.radials.end())
    {
        for (auto &radial : elevationIt->second)
        {
            for (int i = 0; i < radial.gates.size(); i++)
            {
                auto &gate = radial.gates[i];
                if (!std::isnan(gate.reflectivity) && gate.reflectivity > 15)
                {
                    constexpr float deg2rad = std::numbers::pi / 180.0f;
                    float dist = radial.firstGate + radial.gateSize * i;
                    if (dist > circleRadiusm)
                        continue;
                    float az1 = (90.0f - radial.trueAzimuth - 0.25f) * deg2rad; // half beamwidth
                    float az2 = (90.0f - radial.trueAzimuth + 0.25f) * deg2rad;
                    float nearDist = (dist)*circleRadiuspx / circleRadiusm;
                    float farDist = (dist + radial.gateSize) * circleRadiuspx / circleRadiusm;

                    SDL_FColor color = GetColor(gate.reflectivity);
                    SDL_Vertex verts[4] = {
                        {{circleCenter.x + std::cos(az1) * nearDist, circleCenter.y - std::sin(az1) * nearDist}, color, {0, 0}},
                        {{circleCenter.x + std::cos(az2) * nearDist, circleCenter.y - std::sin(az2) * nearDist}, color, {0, 0}},
                        {{circleCenter.x + std::cos(az1) * farDist, circleCenter.y - std::sin(az1) * farDist}, color, {0, 0}},
                        {{circleCenter.x + std::cos(az2) * farDist, circleCenter.y - std::sin(az2) * farDist}, color, {0, 0}},
                    };
                    int indices[] = {0, 1, 2, 1, 2, 3};
                    SDL_RenderGeometry(renderer, nullptr, verts, 4, indices, 6);
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
        if (e.type == SDL_EVENT_KEY_DOWN)
        {
            if (e.key.key == SDLK_W)
            {
                circleRadiusm -= 1000;
                circleRadiusm = std::clamp(circleRadiusm, 1000.0f, 150000.0f);
            }
            if (e.key.key == SDLK_S)
            {
                circleRadiusm += 1000;
                circleRadiusm = std::clamp(circleRadiusm, 1000.0f, 150000.0f);
            }
            if (e.key.key == SDLK_UP)
            {
                elevationLayer = (elevationLayer + 1) % scan.radials.size();
            }
            if (e.key.key == SDLK_DOWN)
            {
                elevationLayer = (elevationLayer - 1) % scan.radials.size();
            }
        }
    }
}