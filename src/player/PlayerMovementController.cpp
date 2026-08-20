#include "player/PlayerMovementController.hpp"

#include <algorithm>
#include <cmath>

namespace hakui {

namespace {

float sanitizeAxis(float value) noexcept
{
    if (!std::isfinite(value)) {
        return 0.0f;
    }
    return std::clamp(value, -1.0f, 1.0f);
}

float sanitizeDelta(float value, float maximum) noexcept
{
    if (!std::isfinite(value) || value <= 0.0f) {
        return 0.0f;
    }
    return std::min(value, maximum);
}

} // namespace

PlayerMovementController::PlayerMovementController(Config config)
    : config_(config)
{
    config_.walkSpeed = std::max(config_.walkSpeed, 0.0f);
    config_.sprintSpeed = std::max(config_.sprintSpeed, config_.walkSpeed);
    config_.sprintStaminaPerSecond = std::max(config_.sprintStaminaPerSecond, 0.0f);
    config_.staminaRecoveryPerSecond = std::max(config_.staminaRecoveryPerSecond, 0.0f);
    config_.maximumDeltaSeconds = std::max(config_.maximumDeltaSeconds, 0.0f);
}

MovementStep PlayerMovementController::update(
    PlayerState& player,
    const MovementInput& input,
    float deltaSeconds
) const noexcept
{
    MovementStep step;
    const float dt = sanitizeDelta(deltaSeconds, config_.maximumDeltaSeconds);
    if (dt == 0.0f) {
        return step;
    }

    player.stamina = std::clamp(player.stamina, 0.0f, 100.0f);

    if (player.locomotion != LocomotionMode::OnFoot) {
        player.stamina = std::min(
            100.0f,
            player.stamina + config_.staminaRecoveryPerSecond * dt
        );
        return step;
    }

    float right = sanitizeAxis(input.right);
    float forward = sanitizeAxis(input.forward);
    const float magnitude = std::sqrt(right * right + forward * forward);

    if (magnitude <= 0.0001f) {
        player.stamina = std::min(
            100.0f,
            player.stamina + config_.staminaRecoveryPerSecond * dt
        );
        return step;
    }

    if (magnitude > 1.0f) {
        right /= magnitude;
        forward /= magnitude;
    }

    step.sprinting = input.sprint && player.stamina > 0.0f;
    const float speed = step.sprinting ? config_.sprintSpeed : config_.walkSpeed;
    step.distance = speed * dt;
    step.moved = true;

    player.x += right * step.distance;
    player.z += forward * step.distance;
    player.yaw = std::atan2(right, forward);

    if (step.sprinting) {
        player.stamina = std::max(
            0.0f,
            player.stamina - config_.sprintStaminaPerSecond * dt
        );
    } else {
        player.stamina = std::min(
            100.0f,
            player.stamina + config_.staminaRecoveryPerSecond * dt
        );
    }

    return step;
}

const PlayerMovementController::Config&
PlayerMovementController::config() const noexcept
{
    return config_;
}

} // namespace hakui
