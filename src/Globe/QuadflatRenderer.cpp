#include "QuadflatRenderer.hpp"

#include <cmath>
#include <glm/glm.hpp>

QuadflatRenderer::QuadflatRenderer() : quadsphere(1)
{
}

QuadflatRenderer::~QuadflatRenderer()
{
}

QuadflatRenderer::GlobeCoords QuadflatRenderer::ToLatLon(glm::vec3 point)
{
    // assumes point is normalized
    return {glm::degrees(std::asin(point.y)),
            glm::degrees(std::atan2(point.z, point.x))};
}

SDL_FPoint QuadflatRenderer::GlobeToUV(QuadflatRenderer::GlobeCoords coords)
{
    return {(coords.lon + 180.0f) / 360.0f,
            (coords.lat + 90.0f) / 180.0f};
}

void QuadflatRenderer::FixVertices(Triangle &triangle, std::vector<SDL_Vertex> &vertices, std::vector<int> &indices)
{
    auto shiftVertex = [&](int v_idx, float x_offset, float color_offset, bool condition) -> int
    {
        if (condition)
        {
            SDL_Vertex v = vertices[v_idx];
            v.position.x += x_offset;

            // Adjust the UV color map so we don't get a harsh rainbow gradient
            // across the tiny wrapped triangles
            v.color.r += color_offset;

            int new_idx = (int)vertices.size();
            vertices.push_back(v);
            return new_idx;
        }
        return v_idx;
    };

    // 1. Draw the LEFT part of the wrapped triangle
    // Shift the right-side vertices (x > 400) to the left side (-800)
    int left1 = shiftVertex(triangle.v1, -800.0f, -1.0f, vertices[triangle.v1].position.x > 400.0f);
    int left2 = shiftVertex(triangle.v2, -800.0f, -1.0f, vertices[triangle.v2].position.x > 400.0f);
    int left3 = shiftVertex(triangle.v3, -800.0f, -1.0f, vertices[triangle.v3].position.x > 400.0f);
    indices.insert(indices.end(), {left1, left2, left3});

    // 2. Draw the RIGHT part of the wrapped triangle
    // Shift the left-side vertices (x < 400) to the right side (+800)
    int right1 = shiftVertex(triangle.v1, 800.0f, 1.0f, vertices[triangle.v1].position.x < 400.0f);
    int right2 = shiftVertex(triangle.v2, 800.0f, 1.0f, vertices[triangle.v2].position.x < 400.0f);
    int right3 = shiftVertex(triangle.v3, 800.0f, 1.0f, vertices[triangle.v3].position.x < 400.0f);
    indices.insert(indices.end(), {right1, right2, right3});
}

bool QuadflatRenderer::OnSeam(Triangle &triangle, std::vector<SDL_Vertex> &vertices)
{
    float maxU = std::max(std::max(vertices[triangle.v1].position.x,
                                   vertices[triangle.v2].position.x),
                          vertices[triangle.v3].position.x);
    float minU = std::min(std::min(vertices[triangle.v1].position.x,
                                   vertices[triangle.v2].position.x),
                          vertices[triangle.v3].position.x);
    return maxU - minU > 400.0f;
}

void QuadflatRenderer::Update(float deltaTime)
{
    SDL_SetRenderDrawColorFloat(renderer, 0, 0, 0, 1);
    SDL_RenderClear(renderer);

    std::vector<SDL_Vertex> vertices;
    std::vector<int> indices;

    auto triangles = quadsphere.GetTriangles();
    auto points = quadsphere.GetPoints();

    vertices.reserve(points.size());
    indices.reserve(triangles.size());

    for (const auto &point : points)
    {
        GlobeCoords coords = ToLatLon(point.point);
        SDL_FPoint uv = GlobeToUV(coords);
        SDL_FColor color{
            (float)rand() / RAND_MAX,
            (float)rand() / RAND_MAX,
            (float)rand() / RAND_MAX,
            // uv.y,
            // 0.5f,
            1.0f};
        vertices.push_back({{uv.x * 800, uv.y * 600}, color, {0, 0}});
    }

    for (auto triangle : triangles)
    {
        if (OnSeam(triangle, vertices))
        {
            FixVertices(triangle, vertices, indices);
        }
        else
        {
            indices.insert(indices.end(), {triangle.v1, triangle.v2, triangle.v3});
        }
    }

    SDL_RenderGeometry(renderer, nullptr, vertices.data(), vertices.size(), indices.data(), indices.size());

    SDL_RenderPresent(renderer);

    HandleEvents();
}