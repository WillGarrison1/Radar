#include "Quadsphere.hpp"

#include <cmath>
#include <glm/glm.hpp>
#include <unordered_set>
#include <bit>
#include <numeric>
#include <iostream>

CubeFace NormPointToUV(const glm::vec3 &p, float &u, float &v)
{
    auto x = std::abs(p.x);
    auto y = std::abs(p.y);
    auto z = std::abs(p.z);

    CubeFace face;

    if (x >= y && x >= z)
    {
        face = (p.x >= 0) ? POSX : NEGX;
    }
    else if (y >= x && y >= z)
    {
        face = (p.y >= 0) ? POSY : NEGY;
    }
    else
    {
        face = (p.z >= 0) ? POSZ : NEGZ;
    }

    switch (face)
    {
    case POSX:
        u = p.z / x;
        v = p.y / x;
        break;
    case NEGX:
        u = -p.z / x;
        v = p.y / x;
        break;
    case POSY:
        u = p.x / y;
        v = p.z / y;
        break;
    case NEGY:
        u = p.x / y;
        v = -p.z / y;
        break;
    case POSZ:
        u = -p.x / z;
        v = p.y / z;
        break;
    case NEGZ:
        u = p.x / z;
        v = p.y / z;
        break;
    default:
        u = v = 0;
        break;
    }

    u = (u + 1.0f) / 2.0f;
    v = (v + 1.0f) / 2.0f;

    return face;
}

static glm::vec3 UVToCubePoint(CubeFace face, float u, float v)
{
    float nx = u * 2.0f - 1.0f;
    float ny = v * 2.0f - 1.0f;

    switch (face)
    {
    case POSX:
        return glm::vec3(1.0f, ny, nx);
    case NEGX:
        return glm::vec3(-1.0f, ny, -nx);
    case POSY:
        return glm::vec3(nx, 1.0f, ny);
    case NEGY:
        return glm::vec3(nx, -1.0f, -ny);
    case POSZ:
        return glm::vec3(-nx, ny, 1.0f);
    case NEGZ:
        return glm::vec3(-nx, -ny, -1.0f);
    default:
        return glm::vec3(0.0f);
    }
}

Quadsphere::Quadsphere(uint8_t depth)
{
    // clang-format off

    quadNodes.insert(quadNodes.end(),
    {
        {0,0,0,POSX}, // POSX
        {0,0,0,NEGX}, // NEGX
        {0,0,0,POSY}, // POSY
        {0,0,0,NEGY}, // NEGY
        {0,0,0,POSZ}, // POSZ
        {0,0,0,NEGZ}, // NEGZ
    });
    // clang-format on

    for (auto &quad : quadNodes)
    {
        float uMin, uMax, vMin, vMax;
        quad.GetUV(uMin, uMax, vMin, vMax);
        auto v1 = UVToCubePoint(quad.face, uMin, vMin);
        auto v2 = UVToCubePoint(quad.face, uMax, vMin);
        auto v3 = UVToCubePoint(quad.face, uMin, vMax);
        auto v4 = UVToCubePoint(quad.face, uMax, vMax);

        for (auto point : {v1, v2, v3, v4})
        {
            XYZKey key(point.x, point.y, point.z);
            if (!indices.contains(key))
            {
                indices[key] = points.size();
                points.push_back({glm::normalize(point), 0});
            }
        }
    }

    for (uint8_t i = 0; i < depth; i++)
    {
        Subdivide();
    }
    UpdateGeometry();
    std::cout << "Created quadsphere with " << points.size() << " points and " << GetTriangles().size() << " triangles" << std::endl;
}

Quadsphere::~Quadsphere()
{
}

void Quadsphere::GetLeafFaces(QuadNode &face, std::vector<uint32_t> &leafs)
{
    if (!face.subFacesStart)
    {
        leafs.emplace_back(GetIndex(face));
        return;
    }

    for (int i = 0; i < 4; i++)
    {
        auto &tree = GetNode(face.subFacesStart + i);
        GetLeafFaces(tree, leafs);
    }
}

void Quadsphere::UpdateGeometry()
{
    triangles.clear();

    uint32_t numLeafs = quadNodes.size();
    uint32_t prevLayerNum = 6;
    while (numLeafs - prevLayerNum != 0)
    {
        numLeafs -= prevLayerNum;
        prevLayerNum *= 4;
    }

    std::vector<uint32_t> leafs;

    triangles.reserve(numLeafs * 2);
    leafs.reserve(numLeafs);

    // Get the leaf nodes for the 6 root faces
    GetLeafFaces(GetNode(POSX), leafs);
    GetLeafFaces(GetNode(NEGX), leafs);
    GetLeafFaces(GetNode(POSY), leafs);
    GetLeafFaces(GetNode(NEGY), leafs);
    GetLeafFaces(GetNode(POSZ), leafs);
    GetLeafFaces(GetNode(NEGZ), leafs);

    for (auto leaf : leafs)
    {
        auto &quad = GetNode(leaf);

        float uMin, uMax, vMin, vMax;
        quad.GetUV(uMin, uMax, vMin, vMax);

        auto v1 = UVToCubePoint(quad.face, uMin, vMin);
        auto v2 = UVToCubePoint(quad.face, uMax, vMin);
        auto v3 = UVToCubePoint(quad.face, uMin, vMax);
        auto v4 = UVToCubePoint(quad.face, uMax, vMax);

        XYZKey v1Key(v1.x, v1.y, v1.z);
        XYZKey v2Key(v2.x, v2.y, v2.z);
        XYZKey v3Key(v3.x, v3.y, v3.z);
        XYZKey v4Key(v4.x, v4.y, v4.z);

        triangles.emplace_back(indices[v1Key], indices[v2Key], indices[v3Key]);
        triangles.emplace_back(indices[v2Key], indices[v3Key], indices[v4Key]);
    }
}

QuadNode &Quadsphere::GetFace(const glm::vec3 &point)
{
    float u, v;
    auto face = NormPointToUV(point, u, v);

    auto faceRoot = GetNode(face);
    return GetChild(faceRoot, u, v);
}

QuadNode &Quadsphere::GetChild(QuadNode &node, float u, float v)
{
    if (!node.subFacesStart)
    {
        return node;
    }

    uint8_t level = node.level + 1;
    uint8_t gridSize = 1 << level;
    uint32_t x = u * gridSize;
    uint32_t y = v * gridSize;

    uint32_t childX = x - node.x * 2;
    uint32_t childY = y - node.y * 2;

    return GetChild(GetNode(node.subFacesStart + childY * 2 + childX), u, v);
}

glm::vec3 Quadsphere::GetMidpoint(glm::vec3 a, glm::vec3 b)
{
    return glm::normalize((a + b) / 2.0f);
}

bool Quadsphere::HasPoint(QuadNode &t, const glm::vec3 &point)
{
    float u, v;
    auto face = NormPointToUV(point, u, v);

    if (t.face != face)
    {
        return false;
    }

    float size = 1.0f / (1 << t.level);

    float uMin = t.x * size;
    float uMax = (t.x + 1) * size;
    float vMin = t.y * size;
    float vMax = (t.y + 1) * size;

    return uMax >= u && u >= uMin &&
           vMax >= v && v >= vMin;
}

void Quadsphere::Subdivide(QuadNode &face)
{
    if (face.subFacesStart) // go to child faces
    {
        uint32_t index = GetIndex(face); // use indexes because references can become invalidated after calls to subdivide
        Subdivide(quadNodes[GetNode(index).subFacesStart]);
        Subdivide(quadNodes[GetNode(index).subFacesStart + 1]);
        Subdivide(quadNodes[GetNode(index).subFacesStart + 2]);
        Subdivide(quadNodes[GetNode(index).subFacesStart + 3]);
        return;
    }

    QuadNode topLeft(face.x * 2, face.y * 2, face.level + 1, face.face);
    QuadNode topRight(face.x * 2 + 1, face.y * 2, face.level + 1, face.face);
    QuadNode bottomLeft(face.x * 2, face.y * 2 + 1, face.level + 1, face.face);
    QuadNode bottomRight(face.x * 2 + 1, face.y * 2 + 1, face.level + 1, face.face);

    face.subFacesStart = quadNodes.size();

    // add new points (if needed)
    float uMin, uMax, vMin, vMax;
    face.GetUV(uMin, uMax, vMin, vMax);
    auto v1 = UVToCubePoint(face.face, (uMin + uMax) / 2, vMin);
    auto v2 = UVToCubePoint(face.face, uMax, (vMin + vMax) / 2);
    auto v3 = UVToCubePoint(face.face, (uMin + uMax) / 2, vMax);
    auto v4 = UVToCubePoint(face.face, uMin, (vMin + vMax) / 2);
    auto v5 = UVToCubePoint(face.face, (uMin + uMax) / 2, (vMin + vMax) / 2);

    for (auto point : {v1, v2, v3, v4, v5})
    {
        XYZKey key(point.x, point.y, point.z);
        if (!indices.contains(key))
        {
            indices[key] = points.size();
            points.push_back({glm::normalize(point), 0});
        }
    }

    quadNodes.insert(quadNodes.end(), {topLeft, topRight, bottomLeft, bottomRight});
}

void Quadsphere::Subdivide()
{
    auto numPoints = points.size();

    int sum = 6;
    int leafs = 6;
    while (sum <= quadNodes.size())
    {
        leafs *= 4;
        sum += leafs;
    }

    points.reserve(sum * 4);
    quadNodes.reserve(sum);

    Subdivide(quadNodes[0]);
    Subdivide(quadNodes[1]);
    Subdivide(quadNodes[2]);
    Subdivide(quadNodes[3]);
    Subdivide(quadNodes[4]);
    Subdivide(quadNodes[5]);
}