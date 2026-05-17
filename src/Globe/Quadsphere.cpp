#include "Quadsphere.hpp"

#include <cmath>
#include <glm/glm.hpp>
#include <unordered_set>
#include <bit>
#include <numeric>
#include <iostream>

Quadsphere::Quadsphere(uint8_t depth)
{
    // clang-format off
    points = {
    {{1, 1, 1},0},{{-1, 1, 1},0},{{1, 1, -1},0},{{-1, 1, -1},0},
    {{1, -1, 1},0},{{-1, -1, 1},0},{{1, -1, -1},0},{{-1, -1, -1},0}
    };

    quadNodes.insert(quadNodes.end(),
    {
        {3,2,0,1},{1,0,4,5},{3,1,5,7},{2,3,7,6},
        {0,2,6,4},{7,6,4,5}
    });
    // clang-format on

    for (int i = 0; i < 8; i++)
    {
        auto &v = points[i];
        v.point = glm::normalize(v.point);
        LatLonCoords latLon(v.point);
        XYKey key(latLon.lon, latLon.lat);
        indices[key] = i;
    }

    for (uint8_t i = 0; i < depth; i++)
    {
        Subdivide();
        std::cout << "Subdividing quadsphere: " << points.size() << " points - " << GetTriangles().size() << " triangles" << std::endl;
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
        return;
    }

    for (int i = 0; i < 4; i++)
    {
        auto &tree = GetNode(face.subFaces[i]);
        GetLeafFaces(tree, leafs);
    }
}

std::vector<Triangle> Quadsphere::GetTriangles()
{

    uint32_t numLeafs = quadNodes.size();
    uint32_t prevLayerNum = 6;
    while (numLeafs - prevLayerNum != 0)
    {
        numLeafs -= prevLayerNum;
        prevLayerNum *= 4;
    }

    std::vector<Triangle> triangles;
    std::vector<uint32_t> leafs;

    triangles.reserve(numLeafs * 2);
    leafs.reserve(numLeafs);

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
        triangles.push_back({quad.v1, quad.v2, quad.v4});
        triangles.push_back({quad.v2, quad.v3, quad.v4});
    }

    return triangles;
}

QuadNode &Quadsphere::GetFace(const glm::vec3 &point)
{
    QuadNode &rootNode = quadNodes[0];
    for (int i = 1; i < 6; i++)
    {
        if (HasPoint(quadNodes[i], point))
        {
            rootNode = quadNodes[i];
        }
    }

    bool f = HasPoint(rootNode, point);

    while (true)
    {
        QuadNode &child = GetChild(rootNode, point);
        if (child == rootNode)
        {
            break;
        }
        rootNode = child;
    }

    return rootNode;
}

QuadNode &Quadsphere::GetChild(const QuadNode &node, glm::vec3 point)
{
    float u, v;
    node.NormPointToUV(point, u, v);

    
}

glm::vec3 Quadsphere::GetMidpoint(glm::vec3 a, glm::vec3 b)
{
    return glm::normalize((a + b) / 2.0f);
}

bool Quadsphere::HasPoint(QuadNode &t, const glm::vec3 &point)
{
    float u, v;
    t.NormPointToUV(point, u, v);

    float uV1, vV1;
    t.NormPointToUV(points[t.face.v1].point, uV1, vV1);

    float uV2, vV2;
    t.NormPointToUV(points[t.face.v2].point, uV2, vV2);

    float uV3, vV3;
    t.NormPointToUV(points[t.face.v3].point, uV3, vV3);

    float uV4, vV4;
    t.NormPointToUV(points[t.face.v4].point, uV4, vV4);

    float uFace[4] = {uV1, uV2, uV3, uV4};
    float vFace[4] = {vV1, vV2, vV3, vV4};

    float uMin = *std::min_element(uFace, uFace + 4);
    float uMax = *std::max_element(uFace, uFace + 4);
    float vMin = *std::min_element(vFace, vFace + 4);
    float vMax = *std::max_element(vFace, vFace + 4);

    return uMax >= u && u >= uMin &&
           vMax >= v && v >= vMin;
}

void Quadsphere::Subdivide(QuadNode &face)
{
    if (face.subFaces[0]) // go to child faces
    {
        Subdivide(quadNodes[face.subFaces[0]]);
        Subdivide(quadNodes[face.subFaces[1]]);
        Subdivide(quadNodes[face.subFaces[2]]);
        Subdivide(quadNodes[face.subFaces[3]]);
        return;
    }

    auto [v1Index, v2Index, v3Index, v4Index] = face.face;

    auto v1 = points[v1Index].point;
    auto v2 = points[v2Index].point;
    auto v3 = points[v3Index].point;
    auto v4 = points[v4Index].point;

    glm::vec3 midpoints[5];

    midpoints[0] = glm::normalize(GetMidpoint(v1, v2));
    midpoints[1] = glm::normalize(GetMidpoint(v2, v3));
    midpoints[2] = glm::normalize(GetMidpoint(v3, v4));
    midpoints[3] = glm::normalize(GetMidpoint(v4, v1));
    midpoints[4] = glm::normalize(GetMidpoint(v2, v4));

    int midpointIndices[5] = {0};
    for (int i = 0; i < sizeof(midpoints) / sizeof(midpoints[0]); i++)
    {
        auto point = midpoints[i];
        LatLonCoords latLon(point);
        XYKey key(latLon.lon, latLon.lat);
        if (!indices.contains(key))
        {
            indices[key] = points.size();
            points.emplace_back(point);
        }
        midpointIndices[i] = indices[key];
    }

    QuadNode topLeft(v1Index, midpointIndices[0], midpointIndices[4], midpointIndices[3]);
    QuadNode topRight(midpointIndices[0], v2Index, midpointIndices[1], midpointIndices[4]);
    QuadNode bottomRight(midpointIndices[4], midpointIndices[1], v3Index, midpointIndices[2]);
    QuadNode bottomLeft(midpointIndices[3], midpointIndices[4], midpointIndices[2], v4Index);

    face.subFaces[0] = quadNodes.size();
    face.subFaces[1] = quadNodes.size() + 1;
    face.subFaces[2] = quadNodes.size() + 2;
    face.subFaces[3] = quadNodes.size() + 3;

    quadNodes.insert(quadNodes.end(), {topLeft, topRight, bottomRight, bottomLeft});
}

void Quadsphere::Subdivide()
{
    auto numPoints = points.size();
    points.reserve(numPoints * 4 - 6);

    int sum = 6;
    int leafs = 6;
    while (sum <= quadNodes.size())
    {
        leafs *= 4;
        sum += leafs;
    }
    quadNodes.reserve(sum);

    Subdivide(quadNodes[0]);
    Subdivide(quadNodes[1]);
    Subdivide(quadNodes[2]);
    Subdivide(quadNodes[3]);
    Subdivide(quadNodes[4]);
    Subdivide(quadNodes[5]);
}