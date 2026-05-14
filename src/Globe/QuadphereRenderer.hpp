#pragma once

#include <SDL3/SDL.h>
#include <Renderer.hpp>
#include "Icosphere.hpp"

class IcosphereRenderer : public Renderer
{
public:
    IcosphereRenderer();
    ~IcosphereRenderer();

    void Update(float deltaTime) override;

private:
    SDL_FPoint WorldToScreen(glm::vec3 world);

    static constexpr float scaleFactor = 100;
    Icosphere icosphere;
    glm::vec3 cameraPos;
};