#pragma once

#include <SDL3/SDL.h>
#include "StormProcessor.hpp"

class RadarRenderer
{
public:
    RadarRenderer();
    ~RadarRenderer();

    void Update();

    inline bool ShouldQuit()
    {
        return shouldQuit;
    }

private:
    SDL_Renderer *renderer;
    SDL_Window *window;
    bool shouldQuit;
    std::map<SampleTimePoint, VolumeScan> cache;

    StormProcessor processor;
};