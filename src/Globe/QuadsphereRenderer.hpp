#pragma once

#include <SDL3/SDL.h>
#include <Renderer.hpp>
#include "Quadsphere.hpp"

class QuadsphereRenderer : public Renderer
{
public:
    QuadsphereRenderer();
    ~QuadsphereRenderer();

    void Update(float deltaTime) override;

private:
    SDL_FPoint WorldToScreen(glm::vec3 world);

    static constexpr float scaleFactor = 100;
    Quadsphere quadsphere;
    glm::vec3 cameraPos;
};