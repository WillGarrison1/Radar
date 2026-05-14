#pragma once

#include <vector>
#include <glm/vec3.hpp>
#include <stdexcept>

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
    int v1, v2, v3, v4;
};

struct QuadNode
{
    uint32_t subFaces[4];
    uint32_t parent;
    QuadFace face;

    QuadNode() = default;
    QuadNode(int a, int b, int c, int d) : subFaces({}),
                                           parent(0),
                                           face({a, b, c, d}) {}
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
    inline uint32_t GetIndex(QuadNode &node)
    {
        return &node - quadNodes.data();
    }
    void Subdivide();

    inline void Subdivide(QuadNode &face)
    {
        std::unordered_map<uint64_t, int> cache;
        Subdivide(face, cache);
    }

    inline QuadNode &GetNode(uint32_t index)
    {
        if (quadNodes.size() <= index)
        {
            throw std::runtime_error("Out of bounds index provided: " + index);
        }
        return quadNodes[index];
    }

private:
    void Subdivide(QuadNode face, std::unordered_map<uint64_t, int> &midpointCache);
    glm::vec3 GetMidpoint(glm::vec3 a, glm::vec3 b);
    uint64_t VertexHash(uint8_t face, uint8_t level, uint32_t x, uint32_t y);

    std::unordered_map<uint64_t, uint32_t> pointsMap;
    std::vector<PointValue> points;
    std::vector<QuadNode> quadNodes;
};