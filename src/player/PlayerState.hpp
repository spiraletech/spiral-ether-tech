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

    // Persistent world transform begins here. These are deliberately plain
    // values for now so networking/save serialization can own them later.
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float yaw = 0.0f;

    // Runtime presentation state. The movement controller supplies intent;
    // the client smooths these values for the procedural debug-avatar pose.
    float movementBlend = 0.0f;
    float gaitPhase = 0.0f;
    bool sprinting = false;

    float health = 100.0f;
    float hunger = 100.0f;
    float stamina = 100.0f;
    float money = 0.0f;
};
