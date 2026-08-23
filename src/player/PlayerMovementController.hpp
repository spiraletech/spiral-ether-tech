#pragma once

#include <span>
#include <optional>

#include "player/PlayerState.hpp"

namespace hakui {

struct MovementInput {
    float right = 0.0f;
    float forward = 0.0f;
    bool sprint = false;
    bool jumpPressed = false;
};

struct MovementStep {
    bool moved = false;
    bool sprinting = false;
    bool jumped = false;
    bool landed = false;
    bool respawned = false;
    float distance = 0.0f;
};

struct HorizontalCollider {
    float minimumX = 0.0f;
    float maximumX = 0.0f;
    float minimumZ = 0.0f;
    float maximumZ = 0.0f;
};

struct WalkableSurface {
    float minimumX = 0.0f;
    float maximumX = 0.0f;
    float minimumZ = 0.0f;
    float maximumZ = 0.0f;
    float baseHeight = 0.0f;
    float slopeX = 0.0f;
    float slopeZ = 0.0f;

    std::optional<float> heightAt(float x, float z) const noexcept;
};

struct MovementEnvironment {
    float floorMinimumX = -10000.0f;
    float floorMaximumX = 10000.0f;
    float floorMinimumZ = -10000.0f;
    float floorMaximumZ = 10000.0f;
    float floorHeight = 0.0f;
    float voidResetHeight = -25.0f;
    float spawnX = 0.0f;
    float spawnY = 0.0f;
    float spawnZ = 0.0f;
    std::span<const HorizontalCollider> colliders{};
    std::span<const WalkableSurface> surfaces{};

    bool hasFloorAt(float x, float z) const noexcept;
    std::optional<float> groundHeightAt(float x, float z) const noexcept;
};

class PlayerMovementController {
public:
    struct Config {
        float walkSpeed = 3.25f;
        float sprintSpeed = 5.75f;
        float groundAcceleration = 26.0f;
        float groundDeceleration = 34.0f;
        float airControl = 0.32f;
        float gravity = 20.0f;
        float jumpVelocity = 7.25f;
        float turnSpeed = 10.0f;
        float collisionRadius = 0.32f;
        float maximumStepHeight = 0.36f;
        float sprintStaminaPerSecond = 12.0f;
        float staminaRecoveryPerSecond = 8.0f;
        float maximumDeltaSeconds = 0.1f;
        float skateboardCruiseSpeed = 4.50f;
        float skateboardSprintSpeed = 7.00f;
        float skateboardAcceleration = 8.0f;
        float skateboardDeceleration = 4.5f;
        float skateboardTurnSpeed = 7.0f;
        float skateboardOllieVelocity = 6.25f;
        float bmxCruiseSpeed = 5.25f;
        float bmxSprintSpeed = 8.50f;
        float bmxAcceleration = 10.0f;
        float bmxDeceleration = 6.0f;
        float bmxTurnSpeed = 6.5f;
        float bmxBunnyHopVelocity = 6.85f;
    };

    PlayerMovementController() = default;
    explicit PlayerMovementController(Config config);

    MovementStep update(
        PlayerState& player,
        const MovementInput& input,
        float deltaSeconds
    ) const noexcept;

    MovementStep update(
        PlayerState& player,
        const MovementInput& input,
        const MovementEnvironment& environment,
        float deltaSeconds
    ) const noexcept;

    const Config& config() const noexcept;

private:
    Config config_;
};

} // namespace hakui
