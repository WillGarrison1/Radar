#include "Icosphere.hpp"

#include <cmath>
#include <glm/glm.hpp>
#include <unordered_set>
#include <bit>
#include <iostream>

Icosphere::Icosphere(uint8_t depth)
{
    float t = (1.0f + std::sqrt(5.0f)) / 2.0f; // golden ratio

    // clang-format off
    points = {
    {-1,  t,  0}, { 1,  t,  0}, {-1, -t,  0}, { 1, -t,  0},
    { 0, -1,  t}, { 0,  1,  t}, { 0, -1, -t}, { 0,  1, -t},
    { t,  0, -1}, { t,  0,  1}, {-t,  0, -1}, {-t,  0,  1}
    };

    for (auto& v : points)
        v = glm::normalize(v);

    triangleIndices = {
        {0, 11, 5}, {0, 5, 1},  {0, 1, 7},   {0, 7, 10}, {0, 10, 11},
        {1, 5, 9},  {5, 11, 4}, {11, 10, 2}, {10 ,7, 6}, {7, 1, 8},
        {3, 9, 4},  {3, 4, 2},  {3, 2, 6},   {3, 6, 8},  {3, 8, 9},
        {4, 9, 5},  {2, 4, 11}, {6, 2, 10},  {8, 6, 7},  {9, 8, 1},
    };
    // clang-format on

    for (uint8_t i = 0; i < depth; i++)
    {
        Subdivide();
    }
    std::cout << "Created Icosphere with " << points.size() << " points" << std::endl;
}

glm::vec3 Icosphere::GetMidpoint(glm::vec3 a, glm::vec3 b)
{
    return glm::normalize((a + b) / 2.0f);
}

uint64_t Icosphere::MidpointHash(int a, int b)
{
    if (a > b)
        std::swap(a, b);
    return (static_cast<uint64_t>(a) << 32ULL) | static_cast<uint64_t>(b);
}

void Icosphere::Subdivide()
{
    std::unordered_map<uint64_t, int> midpointCache;
    std::vector<Triangle> oldTriangles = std::move(triangleIndices);
    triangleIndices.clear();
    for (Triangle &triangle : oldTriangles)
    {
        auto v1 = points[triangle.v1];
        auto v2 = points[triangle.v2];
        auto v3 = points[triangle.v3];

        uint64_t m1Hash = MidpointHash(triangle.v1, triangle.v2);
        uint64_t m2Hash = MidpointHash(triangle.v2, triangle.v3);
        uint64_t m3Hash = MidpointHash(triangle.v3, triangle.v1);
        int currentIndex = points.size();

        if (!midpointCache.contains(m1Hash))
        {
            auto m1 = GetMidpoint(v1, v2);
            midpointCache[m1Hash] = currentIndex++;
            points.push_back(m1);
        }

        if (!midpointCache.contains(m2Hash))
        {
            auto m2 = GetMidpoint(v2, v3);
            midpointCache[m2Hash] = currentIndex++;
            points.push_back(m2);
        }

        if (!midpointCache.contains(m3Hash))
        {
            auto m3 = GetMidpoint(v3, v1);
            midpointCache[m3Hash] = currentIndex++;
            points.push_back(m3);
        }

        int m1Index = midpointCache[m1Hash];
        int m2Index = midpointCache[m2Hash];
        int m3Index = midpointCache[m3Hash];

        triangleIndices.push_back({triangle.v1, m1Index, m3Index});
        triangleIndices.push_back({triangle.v2, m1Index, m2Index});
        triangleIndices.push_back({triangle.v3, m2Index, m3Index});
        triangleIndices.push_back({m1Index, m2Index, m3Index});
    }
}
