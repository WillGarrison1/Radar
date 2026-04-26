#pragma once

#include <SDL3/SDL.h>

class Renderer
{
public:
    Renderer();
    virtual ~Renderer();

    virtual void Update(float deltaTime) = 0;

    inline bool ShouldQuit()
    {
        return hasQuit;
    }

protected:
    void HandleEvents();
    bool hasQuit;
    SDL_Renderer *renderer;
    SDL_Window *window;
};