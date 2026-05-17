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

struct XYZKey
{
    static const int SCALE = 1000000;
    int x, y, z;
    XYZKey() = default;
    XYZKey(float x, float y, float z) : x(std::round(x * SCALE)), y(std::round(y * SCALE)), z(std::round(z * SCALE))
    {
    }

    ~XYZKey() = default;

    constexpr bool operator==(const XYZKey &other) const
    {
        return x == other.x && y == other.y && z == other.z;
    }
};

namespace std
{
    template <>
    struct hash<XYZKey>
    {
        std::size_t operator()(const XYZKey &key) const
        {
            std::size_t h = std::hash<int>()(key.x);

            h ^= std::hash<int>()(key.y) + 0x9e3779b9 + (h << 6) + (h >> 2);

            h ^= std::hash<int>()(key.z) + 0x9e3779b9 + (h << 6) + (h >> 2);

            return h;
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

enum CubeFace : uint8_t
{
    POSX,
    NEGX,
    POSY,
    NEGY,
    POSZ,
    NEGZ
};

#pragma pack(1)
struct QuadNode
{
    uint32_t subFacesStart;
    uint32_t x, y;
    uint8_t level;
    CubeFace face;

    QuadNode() = default;
    QuadNode(uint32_t x, uint32_t y, uint8_t level, CubeFace face) : subFacesStart(0),
                                                                     x(x), y(y),
                                                                     level(level), face(face)
    {
    }

    constexpr bool operator==(const QuadNode &other) const
    {
        return other.x == x && other.y == y && other.level == level && other.face == face;
    }

    inline void GetUV(float &uMin, float &uMax, float &vMin, float &vMax)
    {
        float size = 1.0f / (1 << level);

        uMin = x * size;
        uMax = (x + 1) * size;
        vMin = y * size;
        vMax = (y + 1) * size;
    }
};
#pragma pack()

static int s = sizeof(QuadNode);

class Quadsphere
{
public:
    Quadsphere(uint8_t depth);
    ~Quadsphere();

    inline std::vector<PointValue> &GetPoints()
    {
        return points;
    }

    void UpdateGeometry();

    std::vector<Triangle> &GetTriangles()
    {
        return triangles;
    }

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
            throw std::runtime_error("Out of bounds index provided: " + std::to_string(index));
        }
        return quadNodes.at(index);
    }

private:
    inline glm::vec3 GetMidpoint(glm::vec3 a, glm::vec3 b);
    QuadNode &GetChild(QuadNode &nodes, float u, float v);

    std::unordered_map<XYZKey, int> indices;
    std::vector<PointValue> points;
    std::vector<QuadNode> quadNodes;
    std::vector<Triangle> triangles;
};