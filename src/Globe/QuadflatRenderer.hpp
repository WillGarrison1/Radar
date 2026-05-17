#pragma once

#include <Renderer.hpp>
#include "Quadsphere.hpp"

class QuadflatRenderer : public Renderer
{
public:
    QuadflatRenderer();
    ~QuadflatRenderer();

    void Update(float deltaTime) override;

private:
    struct GlobeCoords
    {
        float lat;
        float lon;
    };

    GlobeCoords ToLatLon(glm::vec3 point);
    SDL_FPoint GlobeToUV(GlobeCoords coords);
    void FixVertices(Triangle &triangle, std::vector<SDL_Vertex> &vertices, std::vector<int> &indices);
    bool OnSeam(Triangle &triangle, std::vector<SDL_Vertex> &vertices);

    Quadsphere quadsphere;
};