#pragma once

#include "player/PlayerState.hpp"

namespace hakui {

struct MovementInput {
    float right = 0.0f;
    float forward = 0.0f;
    bool sprint = false;
};

struct MovementStep {
    bool moved = false;
    bool sprinting = false;
    float distance = 0.0f;
};

class PlayerMovementController {
public:
    struct Config {
        float walkSpeed = 3.25f;
        float sprintSpeed = 5.75f;
        float sprintStaminaPerSecond = 12.0f;
        float staminaRecoveryPerSecond = 8.0f;
        float maximumDeltaSeconds = 0.1f;
    };

    PlayerMovementController() = default;
    explicit PlayerMovementController(Config config);

    MovementStep update(
        PlayerState& player,
        const MovementInput& input,
        float deltaSeconds
    ) const noexcept;

    const Config& config() const noexcept;

private:
    Config config_;
};

} // namespace hakui
