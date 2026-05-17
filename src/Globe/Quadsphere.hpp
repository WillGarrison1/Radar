#pragma once

#include <vector>
#include <glm/vec3.hpp>
#include <stdexcept>
#include <cmath>

struct LatLonCoords
{
    float lat, lon;

    LatLonCoords() = default;
    LatLonCoords(glm::vec3 point)
    {
        lat = std::asin(point.y) * 180 / std::numbers::pi;
        lon = std::atan2(point.z, point.x) * 180 / std::numbers::pi;
    }

    ~LatLonCoords() = default;
};

struct XYKey
{
    uint64_t data;
    XYKey() = default;
    XYKey(float x, float y)
    {
        data = (static_cast<uint64_t>(std::bit_cast<uint32_t>(x)) << 32) |
               (static_cast<uint64_t>(std::bit_cast<uint32_t>(y)));
    }

    ~XYKey() = default;

    constexpr bool operator==(const XYKey &other) const
    {
        return data == other.data;
    }
};

namespace std
{
    template <>
    struct hash<XYKey>
    {
        std::size_t operator()(const XYKey &key) const
        {
            return key.data;
        }
    };
}
struct PointValue
{
    glm::vec3 point;
    float value;
};

struct Triangle
{
    int v1, v2, v3;
};

struct QuadFace
{
    int v1, v2, v3, v4; // top left, top right, bottom right, bottom left
    constexpr bool operator==(const QuadFace &other) const
    {
        return v1 == other.v1 && v2 == other.v2 && v3 == other.v3 && v4 == other.v4;
    }
};

enum DominantCoord
{
    POSX,
    NEGX,
    POSY,
    NEGY,
    POSZ,
    NEGZ
};

struct QuadNode
{
    uint32_t subFaces[4];
    uint32_t parent;
    QuadFace face;

    QuadNode() = default;
    QuadNode(int a, int b, int c, int d) : subFaces({0}),
                                           parent(0),
                                           face({a, b, c, d}) {}

    constexpr bool operator==(const QuadNode &other) const
    {
        return other.face == face;
    }

    void NormPointToUV(const glm::vec3 &p, float &u, float &v) const
    {
        auto x = std::abs(p.x);
        auto y = std::abs(p.y);
        auto z = std::abs(p.z);

        DominantCoord dominant;

        if (x >= y && x >= z)
        {
            dominant = (p.x >= 0) ? POSX : NEGX;
        }
        else if (y >= x && y >= z)
        {
            dominant = (p.y >= 0) ? POSY : NEGY;
        }
        else
        {
            dominant = (p.x >= 0) ? POSZ : NEGZ;
        }

        switch (dominant)
        {
        case POSX:
            u = -p.z / x;
            v = p.y / x;
            break;
        case NEGX:
            u = p.z / x;
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
    }
};

class Quadsphere
{
public:
    Quadsphere(uint8_t depth);
    ~Quadsphere();

    inline std::vector<PointValue> &GetPoints()
    {
        return points;
    }

    std::vector<Triangle> GetTriangles();

    void GetLeafFaces(QuadNode &face, std::vector<uint32_t> &leafs);
    QuadNode &GetFace(const glm::vec3 &point);
    bool HasPoint(QuadNode &t, const glm::vec3 &point);

    inline uint32_t GetIndex(const QuadNode &node)
    {
        auto start = quadNodes.data();
        auto element = &node;
        return element - start;
    }

    void Subdivide();
    void Subdivide(QuadNode &face);

    inline QuadNode &GetNode(uint32_t index)
    {
        if (quadNodes.size() <= index)
        {
            throw std::runtime_error("Out of bounds index provided: " + index);
        }
        return quadNodes.at(index);
    }

private:
    inline glm::vec3 GetMidpoint(glm::vec3 a, glm::vec3 b);
    QuadNode &GetChild(const QuadNode &nodes, glm::vec3 coordinates);

    std::unordered_map<XYKey, int> indices;
    std::vector<PointValue> points;
    std::vector<QuadNode> quadNodes;
};