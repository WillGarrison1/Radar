#include "Renderer.hpp"

#include <stdexcept>

Renderer::Renderer() : hasQuit(false)
{
    if (!SDL_WasInit(SDL_INIT_VIDEO))
    {
        bool success = SDL_Init(SDL_INIT_VIDEO);
        if (!success)
        {
            throw std::runtime_error("Failed to initialize SDL!");
        }
    }
    if (!SDL_CreateWindowAndRenderer("Icosphere", 800, 600, 0, &window, &renderer))
    {
        throw std::runtime_error("Failed to create window and renderer!");
    }
}

Renderer::~Renderer()
{
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
}

void Renderer::HandleEvents()
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        if (e.type == SDL_EVENT_QUIT)
        {
            hasQuit = true;
        }
    }
}