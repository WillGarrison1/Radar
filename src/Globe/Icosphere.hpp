#pragma once

#include <vector>
#include <glm/vec3.hpp>

struct Triangle
{
    int v1, v2, v3; // triangle indices
};

struct FaceTree
{
    FaceTree *subFaces[4];
    FaceTree *parent;
    Triangle face;

    FaceTree() = default;
    FaceTree(int a, int b, int c) : subFaces({}), parent(nullptr), face({a, b, c}) {}
};

struct PointValue
{
    glm::vec3 point;
    float value;
};

class Icosphere
{
public:
    Icosphere(uint8_t depth);
    ~Icosphere();

    inline std::vector<PointValue> &GetPoints()
    {
        return points;
    }

    std::vector<Triangle> GetTriangles();

    size_t GetFace(const glm::vec3 &point);
    size_t GetFace(const glm::vec3 &point, size_t adjacent);
    bool OnFace(Triangle &t, const glm::vec3 &point);

private:
    std::vector<FaceTree> GetLeafFaces(FaceTree* face);
    glm::vec3 GetMidpoint(glm::vec3 a, glm::vec3 b);
    uint64_t MidpointHash(int a, int b);
    void Subdivide(FaceTree *face, std::unordered_map<uint64_t, int> &midpointCache);
    void Subdivide();
    FaceTree* GetNext();
    FaceTree* GetPrev();
    std::vector<PointValue> points;
    std::array<FaceTree, 20> faces;
};