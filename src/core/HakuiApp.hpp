#pragma once

#include <string_view>

#include <SDL3/SDL.h>

#include "avatar/HakuiSkeleton.hpp"
#include "player/PlayerState.hpp"
#include "render/DebugWorldRenderer.hpp"
#include "spiral/SpiralKernel.hpp"
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
    void initSpiralCore();
    void switchLocomotion(LocomotionMode mode, std::string_view label);
    void update(float dt);
    bool render();

    SDL_Window* window_ = nullptr;
    SDL_GPUDevice* gpu_ = nullptr;
    Uint64 previousCounter_ = 0;
    float titleTimer_ = 0.0f;

    // Spiral is the client's orchestration spine. It remains independent from
    // SDL/rendering and from optional legacy avatar backends.
    spiral::SpiralKernel spiral_;
    spiral::RouterBus::ListenerId spiralListener_ = 0;

    HakuiSkeleton avatarSkeleton_;
    DebugWorldRenderer debugRenderer_;
    WorldState world_;
    PlayerState player_;
    LocomotionRouter locomotion_{player_};
};
