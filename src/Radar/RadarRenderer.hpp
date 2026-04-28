#pragma once

#include <SDL3/SDL.h>
#include <Renderer.hpp>
#include "StormCache.hpp"

class RadarRenderer : public Renderer
{
public:
    RadarRenderer();
    ~RadarRenderer();

    void Update(float deltaTime) override;

private:
    void AddRadialGeometry(float radialWidth, Radial &radial, std::vector<SDL_Vertex> &vertices, std::vector<int> &indices);
    void CreateScanGeometry(RadarScan &scan, std::vector<SDL_Vertex> &vertices, std::vector<int> &indices);
    void UpdateEvents();
    void OnKeyPress(SDL_Event &e);

    StormCache processor;

    SDL_FPoint circleCenter = {400, 300};
    float metersPerPixel = 100;
    size_t elevationLayer = 0;
    size_t timePointIndex = 10;
};