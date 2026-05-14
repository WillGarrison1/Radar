#pragma once

#include <vector>
#include <glm/vec3.hpp>

struct PointValue
{
    glm::vec3 point;
    float value;
};

struct Triangle
{
    union
    {
        struct
        {
            int v1, v2, v3; // triangle indices
        };
        int indices[3];
    };

    constexpr bool operator==(Triangle a)
    {
        return v1 == a.v1 && v2 == a.v2 && v3 == a.v3;
    }

    int SharesSide(std::vector<PointValue> &vertices, Triangle &a);
};

struct FaceTree
{
    FaceTree *subFaces[4];
    FaceTree *parent;
    FaceTree *neighbors[3]; // neigboring faces opposite to v1, v2, and v3 respectively
    Triangle face;

    FaceTree() = default;
    FaceTree(int a, int b, int c) : subFaces({}),
                                    parent(nullptr),
                                    neighbors({nullptr}),
                                    face({a, b, c}) {}
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

    FaceTree *GetFace(const glm::vec3 &point);
    FaceTree *GetFace(const glm::vec3 &point, size_t adjacent);
    bool OnFace(Triangle &t, const glm::vec3 &point);
    void Subdivide();

    inline void Subdivide(FaceTree *face)
    {
        std::unordered_map<uint64_t, int> cache;
        Subdivide(face, cache);
    }

private:
    void Subdivide(FaceTree *face, std::unordered_map<uint64_t, int> &midpointCache);
    std::vector<FaceTree *> GetLeafFaces(FaceTree *face);
    glm::vec3 GetMidpoint(glm::vec3 a, glm::vec3 b);
    uint64_t MidpointHash(int a, int b);
    FaceTree *GetNext(FaceTree *current);
    FaceTree *GetPrev(FaceTree *current);
    std::vector<PointValue> points;
    std::array<FaceTree, 20> faces;
};