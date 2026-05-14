#include "Quadsphere.hpp"

#include <cmath>
#include <glm/glm.hpp>
#include <unordered_set>
#include <bit>
#include <numeric>
#include <iostream>

Quadsphere::Quadsphere(uint8_t depth)
{
    float t = (1.0f + std::sqrt(5.0f)) / 2.0f; // golden ratio

    // clang-format off
    points = {
    {{-1,  t,  0},0}, {{ 1,  t,  0},0}, {{-1, -t,  0},0}, {{ 1, -t,  0},0},
    {{ 0, -1,  t},0}, {{ 0,  1,  t},0}, {{ 0, -1, -t},0}, {{ 0,  1, -t},0},
    {{ t,  0, -1},0}, {{ t,  0,  1},0}, {{-t,  0, -1},0}, {{-t,  0,  1},0}
    };

    for (auto& v : points)
        v.point = glm::normalize(v.point);
    // clang-format on

    for (uint8_t i = 0; i < depth; i++)
    {
        Subdivide();
        std::cout << "Subdividing quadsphere: " << points.size() << " points - " << GetTriangles().size() << " quads" << std::endl;
    }
}

Quadsphere::~Quadsphere()
{
}

void Quadsphere::GetLeafFaces(QuadNode &face, std::vector<uint32_t> &leafs)
{
    if (!face.subFaces[0])
    {
        leafs.emplace_back(GetIndex(face));
    }

    for (int i = 0; i < 4; i++)
    {
        auto tree = GetNode(face.subFaces[i]);
        GetLeafFaces(tree, leafs);
    }
}

std::vector<Triangle> Quadsphere::GetTriangles()
{
    std::vector<Triangle> triangles;

    std::vector<uint32_t> leafs;

    // Get the leaf nodes for the 6 root faces
    GetLeafFaces(GetNode(0), leafs);
    GetLeafFaces(GetNode(1), leafs);
    GetLeafFaces(GetNode(2), leafs);
    GetLeafFaces(GetNode(3), leafs);
    GetLeafFaces(GetNode(4), leafs);
    GetLeafFaces(GetNode(5), leafs);

    for (auto leaf : leafs)
    {
        auto quad = GetNode(leaf).face;
        triangles.push_back({quad.v1, quad.v2, quad.v3});
        triangles.push_back({quad.v2, quad.v3, quad.v4});
    }

    return triangles;
}

QuadNode &Quadsphere::GetFace(const glm::vec3 &point)
{
    return 0;
}

glm::vec3 Quadsphere::GetMidpoint(glm::vec3 a, glm::vec3 b)
{
    return glm::normalize((a + b) / 2.0f);
}

uint64_t Quadsphere::VertexHash(uint8_t face, uint8_t level, uint32_t x, uint32_t y) {
    return ((uint64_t)face  << 61) |
           ((uint64_t)level << 56) |
           ((uint64_t)x    << 28) |
           ((uint64_t)y);
}

void Quadsphere::Subdivide(QuadNode &face, std::unordered_map<uint64_t, int> &midpointCache)
{
    bool hasChild = face.subFaces[0];
    if (hasChild)
    {
        Subdivide(GetNode(face.subFaces[0]), midpointCache);
        Subdivide(GetNode(face.subFaces[1]), midpointCache);
        Subdivide(GetNode(face.subFaces[2]), midpointCache);
        Subdivide(GetNode(face.subFaces[3]), midpointCache);
        return;
    }

    auto &quad = face.face;

    auto v1 = points[quad.v1];
    auto v2 = points[quad.v2];
    auto v3 = points[quad.v3];
    auto v4 = points[quad.v4];

    uint64_t m1Hash = MidpointHash(quad.v1, quad.v2);
    uint64_t m2Hash = MidpointHash(quad.v2, quad.v3);
    uint64_t m3Hash = MidpointHash(quad.v3, quad.v4);
    uint64_t m4Hash = MidpointHash(quad.v4, quad.v1);
    int currentIndex = points.size();

    if (!midpointCache.contains(m1Hash))
    {
        auto m1 = GetMidpoint(v1.point, v2.point);
        midpointCache[m1Hash] = currentIndex++;
        points.push_back({m1, 0});
    }

    if (!midpointCache.contains(m2Hash))
    {
        auto m2 = GetMidpoint(v2.point, v3.point);
        midpointCache[m2Hash] = currentIndex++;
        points.push_back({m2, 0});
    }

    if (!midpointCache.contains(m3Hash))
    {
        auto m3 = GetMidpoint(v3.point, v1.point);
        midpointCache[m3Hash] = currentIndex++;
        points.push_back({m3, 0});
    }

    if (!midpointCache.contains(m4Hash))
    {
        auto m4 = GetMidpoint(v4.point, v1.point);
        midpointCache[m3Hash] = currentIndex++;
        points.push_back({m4, 0});
    }

    int m1Index = midpointCache[m1Hash];
    int m2Index = midpointCache[m2Hash];
    int m3Index = midpointCache[m3Hash];
    int m4Index = midpointCache[m4Hash];

    quadNodes.emplace_back({triangle.v1, m1Index, m3Index});
    quadNodes.emplace_back({triangle.v2, m1Index, m2Index});
    quadNodes.emplace_back({triangle.v3, m2Index, m3Index});
    quadNodes.emplace_back({m1Index, m2Index, m3Index});

    face.subFaces[0] =
        face.subFaces[1] =
            face.subFaces[2] =
                face.subFaces[3] =
                    for (auto &subFace : face->subFaces)
    {
        subFace->parent = face;
    }
}

void Quadsphere::Subdivide()
{
    auto numPoints = points.size();
    points.reserve(numPoints * 4 - 6);

    std::unordered_map<uint64_t, int> cache;
    for (QuadNode &face : faces)
    {
        Subdivide(&face, cache);
    }
}