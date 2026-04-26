#pragma once

#include <vector>
#include <glm/vec3.hpp>

struct Triangle
{
    int v1, v2, v3; // triangle indices
};

class Icosphere
{
public:
    Icosphere(uint8_t depth);
    ~Icosphere() = default;

    inline std::vector<glm::vec3> &GetPoints()
    {
        return points;
    }

    inline std::vector<Triangle>& GetTriangles()
    {
        return triangleIndices;
    }

private:
    glm::vec3 GetMidpoint(glm::vec3 a, glm::vec3 b);
    uint64_t MidpointHash(int a, int b);
    void Subdivide();
    std::vector<glm::vec3> points;
    std::vector<Triangle> triangleIndices;
};