#pragma once

#include <cstdint>
#include <string>

enum class LocomotionMode {
    OnFoot,
    Skateboard,
    BMX,
    Car
};

enum class PlayerActivity {
    Roaming,
    CouchSeated,
    CasinoSeated
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

    // Deterministic movement state. Keeping velocity beside the transform lets
    // the dependency-free gameplay layer own acceleration, jumping, landing,
    // and void recovery without depending on SDL or the renderer.
    float velocityX = 0.0f;
    float velocityY = 0.0f;
    float velocityZ = 0.0f;
    bool grounded = true;
    PlayerActivity activity = PlayerActivity::Roaming;
    std::uint32_t activeAffordanceId = 0;
    std::uint32_t activeSeatAnchorId = 0;
    float seatAnchorError = 0.0f;
    bool seatOccupancy = false;
    std::uint32_t voidRespawns = 0;

    // Runtime presentation state. The movement controller supplies intent;
    // the client smooths these values for the procedural debug-avatar pose.
    float movementBlend = 0.0f;
    float gaitPhase = 0.0f;
    float idlePhase = 0.0f;
    float turnBlend = 0.0f;
    bool sprinting = false;

    float health = 100.0f;
    float hunger = 100.0f;
    float stamina = 100.0f;
    float money = 0.0f;
};
