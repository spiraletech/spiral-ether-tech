#pragma once

#include <SDL3/SDL.h>

#include "avatar/HakuiSkeleton.hpp"
#include "player/PlayerState.hpp"
#include "systems/LocomotionRouter.hpp"
#include "world/WorldState.hpp"

class HakuiApp {
public:
    bool boot();
    SDL_AppResult handleEvent(const SDL_Event& event);
    SDL_AppResult tick();
    void shutdown();

private:
    bool initPlatform();
    bool initGPU();
    void update(float dt);
    bool render();

    SDL_Window* window_ = nullptr;
    SDL_GPUDevice* gpu_ = nullptr;
    Uint64 previousCounter_ = 0;

    HakuiSkeleton avatarSkeleton_;
    WorldState world_;
    PlayerState player_;
    LocomotionRouter locomotion_{player_};
};
