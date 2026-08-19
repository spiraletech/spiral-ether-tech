#pragma once

#include <string>

enum class LocomotionMode {
    OnFoot,
    Skateboard,
    BMX,
    Car
};

struct PlayerState {
    std::string displayName = "ETHER";
    LocomotionMode locomotion = LocomotionMode::OnFoot;
    float health = 100.0f;
    float hunger = 100.0f;
    float stamina = 100.0f;
    float money = 0.0f;
};
