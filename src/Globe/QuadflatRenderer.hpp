#pragma once

#include <Renderer.hpp>
#include "Icosphere.hpp"

class IcoflatRenderer : public Renderer
{
public:
    IcoflatRenderer();
    ~IcoflatRenderer();

    void Update(float deltaTime) override;

private:
    struct GlobeCoords
    {
        float lat;
        float lon;
    };

    GlobeCoords ToLatLon(glm::vec3 point);
    SDL_FPoint GlobeToUV(GlobeCoords coords);
    void FixVertices(Triangle& triangle, std::vector<SDL_Vertex>& vertices, std::vector<int>& indices);
    bool OnSeam(Triangle& triangle, std::vector<SDL_Vertex>& vertices);

    Icosphere icosphere;
};