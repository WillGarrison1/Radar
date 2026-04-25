#pragma once

#include <SDL3/SDL.h>
#include "StormProcessor.hpp"

class RadarRenderer
{
public:
    RadarRenderer();
    ~RadarRenderer();

    void Update(float deltaTime);

    inline bool ShouldQuit()
    {
        return shouldQuit;
    }

private:
    void AddRadialGeometry(float radialWidth, Radial &radial, std::vector<SDL_Vertex> &vertices, std::vector<int> &indices);
    void CreateScanGeometry(RadarScan &scan, std::vector<SDL_Vertex> &vertices, std::vector<int> &indices);
    void UpdateEvents();
    void OnKeyPress(SDL_Event &e);

    SDL_Renderer *renderer;
    SDL_Window *window;
    bool shouldQuit;
    StormProcessor processor;

    SDL_FPoint circleCenter = {400, 300};
    float metersPerPixel = 100;
    size_t elevationLayer = 0;
    size_t timePointIndex = 10;
};