#include "Icosphere.hpp"

#include <cmath>
#include <glm/glm.hpp>
#include <unordered_set>
#include <bit>
#include <iostream>

int Triangle::SharesSide(std::vector<PointValue> &vertices, Triangle &a)
{
    int sharedPoints[3]{-1};
    int numShared = 0;

    for (int i = 0; i < 3; i++)
    {
        for (int i1 = 0; i1 < 3; i1++)
        {
            if (a.indices[i1] == indices[i])
            {
                sharedPoints[i] = i1;
                numShared++;
            }
        }
    }

    if (numShared == 3)
    {
        std::cout << "wtf" << std::endl;
        return -1;
    }

    if (numShared != 2)
    {
        return -1; // no edges shared
    }

    int side = 0;
    for (int i = 0; i < 3; i++)
    {
        if (sharedPoints[i] == -1)
            return i;
    }
    return -1;
}

Icosphere::Icosphere(uint8_t depth)
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

    faces = {
        FaceTree{0, 11, 5}, FaceTree{0, 5, 1},  FaceTree{0, 1, 7},   FaceTree{0, 7, 10}, FaceTree{0, 10, 11},
        FaceTree{1, 5, 9},  FaceTree{5, 11, 4}, FaceTree{11, 10, 2}, FaceTree{10 ,7, 6}, FaceTree{7, 1, 8},
        FaceTree{3, 9, 4},  FaceTree{3, 4, 2},  FaceTree{3, 2, 6},   FaceTree{3, 6, 8},  FaceTree{3, 8, 9},
        FaceTree{4, 9, 5},  FaceTree{2, 4, 11}, FaceTree{6, 2, 10},  FaceTree{8, 6, 7},  FaceTree{9, 8, 1},
    };
    // clang-format on

    // compute neighbors
    for (FaceTree &face : faces)
    {
        if (face.neighbors[0] && face.neighbors[1] && face.neighbors[2])
            continue;
        for (FaceTree &curFace : faces)
        {
            if (curFace.face == face.face)
                continue;

            int neighbor = face.face.SharesSide(points, curFace.face);
            if (neighbor != -1)
            {
                face.neighbors[neighbor] = &curFace;
            }
        }
    }

    for (uint8_t i = 0; i < depth; i++)
    {
        Subdivide();
        std::cout << "Subdividing icosphere: " << points.size() << " points - " << GetTriangles().size() << " triangles" << std::endl;
    }
}

Icosphere::~Icosphere()
{

    // Iterate previous nodes so we go from bottom to the top instead of top to bottom
    // (so we can delete all nodes safely)
    for (auto &root : faces)
    {
        FaceTree *cur = GetPrev(&root);
        while (cur != &root)
        {
            FaceTree *prev = GetPrev(cur);
            delete cur;
            cur = prev;
        }
    }
}

FaceTree *Icosphere::GetNext(FaceTree *current)
{
    if (current->subFaces[0])
        return current->subFaces[0];

    while (current->parent)
    {
        FaceTree *parent = current->parent;
        int us = 3;
        while (parent->subFaces[us] != current)
            us--;

        if (us < 3)
            return parent->subFaces[us + 1];

        current = current->parent;
    }
    return current;
}

FaceTree *Icosphere::GetPrev(FaceTree *current)
{
    FaceTree *parent = current->parent;
    while (parent)
    {

        int us = 0;
        while (parent->subFaces[us] != current)
            us++;

        if (us > 0) // increment child
        {
            current = parent->subFaces[us - 1];

            // because of adaptive subdivision, not all braches are same length
            // so loop to find smallest child (heh)
            // this happens to be the same logic as finding the smallest child from the root node
            // therefore we can just break here
            break;
        }

        // we are the greatest child, so return the parent next
        return parent;
    }

    // at root node, so find the last node (bottom right of tree)
    while (current->subFaces[3])
    {
        current = current->subFaces[3];
    }
    return current;
}

std::vector<FaceTree *> Icosphere::GetLeafFaces(FaceTree *face)
{
    if (!face->subFaces[0])
        return {face};
    std::vector<FaceTree *> leafs;
    for (int i = 0; i < 4; i++)
    {
        auto tree = face->subFaces[i];
        auto list = GetLeafFaces(tree);
        leafs.insert(leafs.end(), list.begin(), list.end());
    }

    return leafs;
}

std::vector<Triangle> Icosphere::GetTriangles()
{
    std::vector<Triangle> triangles;
    for (auto &face : faces)
    {
        auto leafs = GetLeafFaces(&face);
        for (auto leaf : leafs)
        {
            auto triangle = leaf->face;
            triangles.push_back(triangle);
        }
    }
    return triangles;
}

FaceTree *Icosphere::GetFace(const glm::vec3 &point)
{
    return 0;
}

FaceTree *Icosphere::GetFace(const glm::vec3 &point, size_t adjacent)
{
    return 0;
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

void Icosphere::Subdivide(FaceTree *face, std::unordered_map<uint64_t, int> &midpointCache)
{
    bool hasChild = face->subFaces[0] != nullptr;
    if (hasChild)
    {

        Subdivide(face->subFaces[0], midpointCache);
        Subdivide(face->subFaces[1], midpointCache);
        Subdivide(face->subFaces[2], midpointCache);
        Subdivide(face->subFaces[3], midpointCache);
        return;
    }

    auto &triangle = face->face;

    auto v1 = points[triangle.v1];
    auto v2 = points[triangle.v2];
    auto v3 = points[triangle.v3];

    uint64_t m1Hash = MidpointHash(triangle.v1, triangle.v2);
    uint64_t m2Hash = MidpointHash(triangle.v2, triangle.v3);
    uint64_t m3Hash = MidpointHash(triangle.v3, triangle.v1);
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

    int m1Index = midpointCache[m1Hash];
    int m2Index = midpointCache[m2Hash];
    int m3Index = midpointCache[m3Hash];

    face->subFaces[0] = new FaceTree{triangle.v1, m1Index, m3Index};
    face->subFaces[1] = new FaceTree{triangle.v2, m1Index, m2Index};
    face->subFaces[2] = new FaceTree{triangle.v3, m2Index, m3Index};
    face->subFaces[3] = new FaceTree{m1Index, m2Index, m3Index};

    for (auto &subFace : face->subFaces)
    {
        subFace->parent = face;
    }
}

void Icosphere::Subdivide()
{
    auto numPoints = points.size();
    points.reserve(numPoints * 4 - 6);

    std::unordered_map<uint64_t, int> cache;
    for (FaceTree &face : faces)
    {
        Subdivide(&face, cache);
    }
}