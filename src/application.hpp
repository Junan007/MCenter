#ifndef _APPLICATION_HPP_
#define _APPLICATION_HPP_

#include <SDL2/SDL.h>

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl.h"
#include "imgui/imgui_impl_sdlrenderer.h"
#include "mobilemanager.hpp"


class Application
{
private:
    void init();
    void write_location(bool reset);
public:
    Application();
    ~Application();

    int run(int argc, char* argv[]);
    void shutdown();

private:
    SDL_Window* m_pWindow;
    SDL_Renderer* m_pRenderer;

    MobileManager* m_pManager;
};


#endif