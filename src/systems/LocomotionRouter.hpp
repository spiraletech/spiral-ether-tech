#pragma once

#include <SDL3/SDL_log.h>

#include "player/PlayerState.hpp"

class LocomotionRouter {
public:
    explicit LocomotionRouter(PlayerState& player) : player_(player) {}

    void switchTo(LocomotionMode mode)
    {
        if (player_.locomotion == mode) {
            return;
        }

        player_.locomotion = mode;
        switch (mode) {
            case LocomotionMode::OnFoot: SDL_Log("[HAKUI] locomotion -> ON FOOT"); break;
            case LocomotionMode::Skateboard: SDL_Log("[HAKUI] locomotion -> SKATEBOARD"); break;
            case LocomotionMode::BMX: SDL_Log("[HAKUI] locomotion -> BMX"); break;
            case LocomotionMode::Car: SDL_Log("[HAKUI] locomotion -> CAR"); break;
        }
    }

    void update(float dt)
    {
        (void)dt;
        // Controller dispatch lands here next:
        // onFoot_.update(dt)
        // skateboard_.update(dt)
        // bmx_.update(dt)
        // car_.update(dt)
    }

private:
    PlayerState& player_;
};
