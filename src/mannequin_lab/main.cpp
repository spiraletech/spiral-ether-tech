#define SDL_MAIN_USE_CALLBACKS

#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include "mannequin_lab/MannequinLabApp.hpp"

SDL_AppResult SDL_AppInit(void** appstate, int argc, char** argv)
{
    (void)argc;
    (void)argv;

    auto* app = new MannequinLabApp();
    *appstate = app;
    if (!app->boot()) {
        app->shutdown();
        delete app;
        *appstate = nullptr;
        return SDL_APP_FAILURE;
    }
    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void* appstate, SDL_Event* event)
{
    return static_cast<MannequinLabApp*>(appstate)->handleEvent(*event);
}

SDL_AppResult SDL_AppIterate(void* appstate)
{
    return static_cast<MannequinLabApp*>(appstate)->tick();
}

void SDL_AppQuit(void* appstate, SDL_AppResult result)
{
    (void)result;
    auto* app = static_cast<MannequinLabApp*>(appstate);
    if (app) {
        app->shutdown();
        delete app;
    }
}
