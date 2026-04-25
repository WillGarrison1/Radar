#include "RadarRenderer.hpp"

#include <algorithm>
#include <cmath>
#include <format>
#include <iostream>
#include <stdexcept>

RadarRenderer::RadarRenderer() : shouldQuit(false), renderer(nullptr), window(nullptr), processor({})
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
    if (reflectivity > 60)
    {
        return {1, 0, 1, 1};
    }
    if (reflectivity > 50)
    {
        return {1, 0, 0, 1};
    }
    if (reflectivity > 35)
    {
        return {1, 1, 0, 1};
    }
    if (reflectivity > 20)
    {
        return {0, 1, 0, 1};
    }
    if (reflectivity > 10)
    {
        return {0.3, 0.3, 1, 1};
    }
    return {0, 0, 0, 0};
}

void RadarRenderer::Update(float deltaTime)
{
    // static float circleRadiuspx = 275;
    static float metersPerPixel = 100;
    static size_t elevationLayer = 0;
    static size_t timePointIndex = 10;

    SDL_SetRenderDrawColorFloat(renderer, 0.05, 0.05, 0.05, 1.0);
    SDL_RenderClear(renderer);

    SDL_FPoint circleCenter{400, 300};

    // DrawCircle(renderer, circleCenter, circleRadiuspx, {0, 0, 0, 0});

    // radius of circle is 50km
    SDL_SetRenderDrawColorFloat(renderer, 1, 0, 0, 1);

    auto points = processor.GetTimePoints();
    timePointIndex = std::clamp(timePointIndex, 0ull, points.size() - 1);

    auto timePoint = points[points.size() - timePointIndex - 1];
    auto &cache = processor.GetCached();
    auto cacheSize = cache.size();
    auto pending = processor.GetPending().size();
    if (cacheSize + pending <= timePointIndex)
    {
        for (int i = cacheSize + pending; i <= timePointIndex; i++)
        {
            auto tp = points[points.size() - i - 1];
            std::string s = std::format("{:%Y/%m/%d-%H:%M}", tp);
            std::cout << "\rGetting record (" << i << "/" << timePointIndex << ") at time " << s;
            processor.Process(tp);
        }
    }

    if (cache.contains(timePoint) && !cache[timePoint].radials.empty())
    {

        std::vector<SDL_Vertex> vertices;
        std::vector<int> indices;
        vertices.reserve(400000);
        indices.reserve(600000);

        auto &scan = cache[timePoint];
        auto elevationIt = scan.radials.begin();

        std::advance(elevationIt, elevationLayer % scan.radials.size());

        if (elevationIt != scan.radials.end())
        {
            for (auto &radial : elevationIt->second)
            {
                for (auto &gate : radial.gates)
                {
                    constexpr float deg2rad = std::numbers::pi / 180.0f;
                    auto i = gate.gateNum;
                    float az1 = (90.0f - radial.trueAzimuth - scan.radialWidth / 2) * deg2rad; // half beamwidth
                    float az2 = (90.0f - radial.trueAzimuth + scan.radialWidth / 2) * deg2rad;
                    float az1Cos = std::cos(az1);
                    float az2Cos = std::cos(az2);
                    float az1Sin = std::sin(az1);
                    float az2Sin = std::sin(az2);
                    if (!std::isnan(gate.reflectivity) && gate.reflectivity > 10)
                    {
                        float dist = radial.firstGate + radial.gateSize * i;
                        if (dist > metersPerPixel * 400.0f * std::numbers::sqrt2)
                            continue;
                        float nearDist = dist / metersPerPixel;
                        float farDist = (dist + radial.gateSize) / metersPerPixel;

                        SDL_FColor color = GetColor(gate.reflectivity);
                        int start = vertices.size();
                        vertices.push_back({{circleCenter.x + az1Cos * nearDist, circleCenter.y - az1Sin * nearDist}, color, {0, 0}});
                        vertices.push_back({{circleCenter.x + az2Cos * nearDist, circleCenter.y - az2Sin * nearDist}, color, {0, 0}});
                        vertices.push_back({{circleCenter.x + az1Cos * farDist, circleCenter.y - az1Sin * farDist}, color, {0, 0}});
                        vertices.push_back({{circleCenter.x + az2Cos * farDist, circleCenter.y - az2Sin * farDist}, color, {0, 0}});

                        indices.push_back(start);
                        indices.push_back(start + 1);
                        indices.push_back(start + 2);
                        indices.push_back(start + 1);
                        indices.push_back(start + 2);
                        indices.push_back(start + 3);
                    }
                }
            }
        }
        SDL_RenderGeometry(renderer, nullptr, vertices.data(), vertices.size(), indices.data(), indices.size());
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
                metersPerPixel -= 10;
                metersPerPixel = std::clamp(metersPerPixel, 10.0f, 1500.0f);
            }
            if (e.key.key == SDLK_S)
            {
                metersPerPixel += 10;
                metersPerPixel = std::clamp(metersPerPixel, 10.0f, 1500.0f);
            }
            if (e.key.key == SDLK_UP)
            {
                elevationLayer = (elevationLayer + 1) % 10;
            }
            if (e.key.key == SDLK_DOWN)
            {
                elevationLayer = (elevationLayer - 1) % 10;
            }
            if (e.key.key == SDLK_RIGHT)
            {
                auto points = processor.GetTimePoints();
                timePointIndex = (timePointIndex + 1) % points.size();
                std::cout << timePointIndex << std::endl;
            }
            if (e.key.key == SDLK_LEFT)
            {
                if (timePointIndex != 0)
                    timePointIndex--;
                std::cout << timePointIndex << std::endl;
            }
            if (e.key.key == SDLK_R)
            {
                std::cout << "Refreshing...";
                processor.Refresh();
                std::cout << "Done!   Number of Scans: " << processor.GetTimePoints().size() << std::endl;
            }
        }
    }
}