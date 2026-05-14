#include "IcosphereRenderer.hpp"

#include <stdexcept>
#include <algorithm>
#include <random>

float AvgZ(const Triangle &triangle, const std::vector<PointValue> &points, glm::vec3 camPos)
{
    auto v1 = points[triangle.v1].point - camPos;
    auto v2 = points[triangle.v2].point - camPos;
    auto v3 = points[triangle.v3].point - camPos;

    return (v1.z + v2.z + v3.z) / 3.0f;
}

IcosphereRenderer::IcosphereRenderer() : cameraPos({0, 0, -2}), icosphere(6)
{
}

IcosphereRenderer::~IcosphereRenderer()
{
}

SDL_FPoint IcosphereRenderer::WorldToScreen(glm::vec3 world)
{
    glm::vec3 relativePoint = world - cameraPos;
    if (relativePoint.z <= 0)
        return {-1, -1};
    SDL_FPoint p{relativePoint.x / relativePoint.z * scaleFactor + 400, 300 - relativePoint.y / relativePoint.z * scaleFactor};
    return p;
}

void IcosphereRenderer::Update(float deltaTime)
{
    SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, 1);
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColorFloat(renderer, 0, 0, 1, 1);

    auto points = icosphere.GetPoints();

    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;

    for (auto point : points)
    {
        SDL_FColor color{
            static_cast<float>(rand()) / static_cast<float>(RAND_MAX),
            static_cast<float>(rand()) / static_cast<float>(RAND_MAX),
            static_cast<float>(rand()) / static_cast<float>(RAND_MAX), 1};
        SDL_Vertex vert{WorldToScreen(point.point), color, {0, 0}};
        vertices.push_back(vert);
    }

    auto triangles = icosphere.GetTriangles();

    std::sort(triangles.begin(), triangles.end(), [&](const Triangle &a, const Triangle &b)
              {
                  float za = AvgZ(a, points, cameraPos);
                  float zb = AvgZ(b, points, cameraPos);
                  return za > zb; // furthest first
              });

    for (auto triangle : triangles)
    {
        indices.insert(indices.end(), {triangle.v1, triangle.v2, triangle.v3});
    }

    SDL_RenderGeometry(renderer, nullptr, vertices.data(), vertices.size(), indices.data(), indices.size());

    SDL_RenderPresent(renderer);
    HandleEvents();
}