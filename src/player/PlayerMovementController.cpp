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

float approach(float current, float target, float maximumDelta) noexcept
{
    if (current < target) {
        return std::min(current + maximumDelta, target);
    }
    return std::max(current - maximumDelta, target);
}

float wrapAngle(float radians) noexcept
{
    constexpr float pi = 3.14159265358979323846f;
    constexpr float twoPi = pi * 2.0f;
    while (radians > pi) {
        radians -= twoPi;
    }
    while (radians < -pi) {
        radians += twoPi;
    }
    return radians;
}

struct ModeMotionProfile {
    bool enabled = false;
    bool canJump = false;
    float cruiseSpeed = 0.0f;
    float sprintSpeed = 0.0f;
    float acceleration = 0.0f;
    float deceleration = 0.0f;
    float turnSpeed = 0.0f;
    float collisionRadius = 0.32f;
    float jumpVelocity = 0.0f;
};

ModeMotionProfile profileFor(
    LocomotionMode mode,
    const PlayerMovementController::Config& config
) noexcept
{
    switch (mode) {
        case LocomotionMode::OnFoot:
            return {
                true,
                true,
                config.walkSpeed,
                config.sprintSpeed,
                config.groundAcceleration,
                config.groundDeceleration,
                config.turnSpeed,
                config.collisionRadius,
                config.jumpVelocity
            };
        case LocomotionMode::Skateboard:
            return {
                true,
                true,
                config.skateboardCruiseSpeed,
                config.skateboardSprintSpeed,
                config.skateboardAcceleration,
                config.skateboardDeceleration,
                config.skateboardTurnSpeed,
                config.collisionRadius + 0.04f,
                config.skateboardOllieVelocity
            };
        case LocomotionMode::BMX:
            return {
                true,
                true,
                config.bmxCruiseSpeed,
                config.bmxSprintSpeed,
                config.bmxAcceleration,
                config.bmxDeceleration,
                config.bmxTurnSpeed,
                config.collisionRadius + 0.10f,
                config.bmxBunnyHopVelocity
            };
        case LocomotionMode::Car:
            return {};
    }
    return {};
}

} // namespace

bool MovementEnvironment::hasFloorAt(float x, float z) const noexcept
{
    return groundHeightAt(x, z).has_value();
}

std::optional<float> WalkableSurface::heightAt(float x, float z) const noexcept
{
    if (!std::isfinite(x) || !std::isfinite(z) ||
        x < minimumX || x > maximumX || z < minimumZ || z > maximumZ) {
        return std::nullopt;
    }
    return baseHeight + (x - minimumX) * slopeX + (z - minimumZ) * slopeZ;
}

std::optional<float> MovementEnvironment::groundHeightAt(
    float x,
    float z
) const noexcept
{
    if (!std::isfinite(x) || !std::isfinite(z)) {
        return std::nullopt;
    }

    std::optional<float> height;
    if (x >= floorMinimumX && x <= floorMaximumX &&
        z >= floorMinimumZ && z <= floorMaximumZ) {
        height = floorHeight;
    }

    for (const WalkableSurface& surface : surfaces) {
        const std::optional<float> candidate = surface.heightAt(x, z);
        if (candidate && (!height || *candidate > *height)) {
            height = candidate;
        }
    }
    return height;
}

PlayerMovementController::PlayerMovementController(Config config)
    : config_(config)
{
    config_.walkSpeed = std::max(config_.walkSpeed, 0.0f);
    config_.sprintSpeed = std::max(config_.sprintSpeed, config_.walkSpeed);
    config_.groundAcceleration = std::max(config_.groundAcceleration, 0.0f);
    config_.groundDeceleration = std::max(config_.groundDeceleration, 0.0f);
    config_.airControl = std::clamp(config_.airControl, 0.0f, 1.0f);
    config_.gravity = std::max(config_.gravity, 0.0f);
    config_.jumpVelocity = std::max(config_.jumpVelocity, 0.0f);
    config_.turnSpeed = std::max(config_.turnSpeed, 0.0f);
    config_.collisionRadius = std::max(config_.collisionRadius, 0.0f);
    config_.maximumStepHeight = std::max(config_.maximumStepHeight, 0.0f);
    config_.sprintStaminaPerSecond = std::max(config_.sprintStaminaPerSecond, 0.0f);
    config_.staminaRecoveryPerSecond = std::max(config_.staminaRecoveryPerSecond, 0.0f);
    config_.maximumDeltaSeconds = std::max(config_.maximumDeltaSeconds, 0.0f);
    config_.skateboardCruiseSpeed = std::max(config_.skateboardCruiseSpeed, 0.0f);
    config_.skateboardSprintSpeed = std::max(
        config_.skateboardSprintSpeed,
        config_.skateboardCruiseSpeed
    );
    config_.skateboardAcceleration = std::max(config_.skateboardAcceleration, 0.0f);
    config_.skateboardDeceleration = std::max(config_.skateboardDeceleration, 0.0f);
    config_.skateboardTurnSpeed = std::max(config_.skateboardTurnSpeed, 0.0f);
    config_.skateboardOllieVelocity = std::max(
        config_.skateboardOllieVelocity,
        0.0f
    );
    config_.bmxCruiseSpeed = std::max(config_.bmxCruiseSpeed, 0.0f);
    config_.bmxSprintSpeed = std::max(config_.bmxSprintSpeed, config_.bmxCruiseSpeed);
    config_.bmxAcceleration = std::max(config_.bmxAcceleration, 0.0f);
    config_.bmxDeceleration = std::max(config_.bmxDeceleration, 0.0f);
    config_.bmxTurnSpeed = std::max(config_.bmxTurnSpeed, 0.0f);
    config_.bmxBunnyHopVelocity = std::max(
        config_.bmxBunnyHopVelocity,
        0.0f
    );
}

MovementStep PlayerMovementController::update(
    PlayerState& player,
    const MovementInput& input,
    float deltaSeconds
) const noexcept
{
    return update(player, input, MovementEnvironment{}, deltaSeconds);
}

MovementStep PlayerMovementController::update(
    PlayerState& player,
    const MovementInput& input,
    const MovementEnvironment& environment,
    float deltaSeconds
) const noexcept
{
    MovementStep step;
    const float dt = sanitizeDelta(deltaSeconds, config_.maximumDeltaSeconds);
    if (dt == 0.0f) {
        return step;
    }

    player.stamina = std::clamp(player.stamina, 0.0f, 100.0f);

    const ModeMotionProfile motion = profileFor(player.locomotion, config_);
    if (!motion.enabled || player.activity != PlayerActivity::Roaming) {
        player.velocityX = 0.0f;
        player.velocityY = 0.0f;
        player.velocityZ = 0.0f;
        player.stamina = std::min(
            100.0f,
            player.stamina + config_.staminaRecoveryPerSecond * dt
        );
        return step;
    }

    float right = sanitizeAxis(input.right);
    float forward = sanitizeAxis(input.forward);
    const float magnitude = std::sqrt(right * right + forward * forward);

    if (magnitude > 1.0f) {
        right /= magnitude;
        forward /= magnitude;
    }

    const bool hasMovementInput = magnitude > 0.0001f;
    step.sprinting = hasMovementInput && input.sprint && player.stamina > 0.0f;
    const float speed = step.sprinting ? motion.sprintSpeed : motion.cruiseSpeed;
    const float targetVelocityX = hasMovementInput ? right * speed : 0.0f;
    const float targetVelocityZ = hasMovementInput ? forward * speed : 0.0f;
    const float response = hasMovementInput
        ? motion.acceleration * (player.grounded ? 1.0f : config_.airControl)
        : motion.deceleration * (player.grounded ? 1.0f : 0.06f);

    player.velocityX = approach(player.velocityX, targetVelocityX, response * dt);
    player.velocityZ = approach(player.velocityZ, targetVelocityZ, response * dt);

    if (hasMovementInput) {
        const float targetYaw = std::atan2(right, forward);
        const float turnDelta = wrapAngle(targetYaw - player.yaw);
        const float appliedTurn = std::clamp(
            turnDelta,
            -motion.turnSpeed * dt,
            motion.turnSpeed * dt
        );
        player.yaw = wrapAngle(player.yaw + appliedTurn);
        player.turnBlend = std::clamp(
            std::fabs(turnDelta) / 1.57079632679f,
            0.0f,
            1.0f
        );
    } else {
        player.turnBlend = approach(player.turnBlend, 0.0f, 5.0f * dt);
    }

    const std::optional<float> startingGround =
        environment.groundHeightAt(player.x, player.z);
    const float referenceGround = startingGround.value_or(player.y);

    const auto blocked = [&](float x, float z) {
        for (const HorizontalCollider& collider : environment.colliders) {
            if (x > collider.minimumX - motion.collisionRadius &&
                x < collider.maximumX + motion.collisionRadius &&
                z > collider.minimumZ - motion.collisionRadius &&
                z < collider.maximumZ + motion.collisionRadius) {
                return true;
            }
        }
        const std::optional<float> proposedGround = environment.groundHeightAt(x, z);
        if (player.grounded && proposedGround &&
            *proposedGround > referenceGround + config_.maximumStepHeight) {
            return true;
        }
        return false;
    };

    const float startX = player.x;
    const float startZ = player.z;
    const float proposedX = player.x + player.velocityX * dt;
    if (!blocked(proposedX, player.z)) {
        player.x = proposedX;
    } else {
        player.velocityX = 0.0f;
    }
    const float proposedZ = player.z + player.velocityZ * dt;
    if (!blocked(player.x, proposedZ)) {
        player.z = proposedZ;
    } else {
        player.velocityZ = 0.0f;
    }

    const float travelledX = player.x - startX;
    const float travelledZ = player.z - startZ;
    step.distance = std::sqrt(travelledX * travelledX + travelledZ * travelledZ);
    step.moved = step.distance > 0.00001f;

    const std::optional<float> groundBelow =
        environment.groundHeightAt(player.x, player.z);
    const bool floorBelow = groundBelow.has_value();
    if (player.grounded) {
        if (!floorBelow || *groundBelow < referenceGround - config_.maximumStepHeight) {
            player.grounded = false;
            player.y = referenceGround;
        } else {
            player.y = *groundBelow;
        }
    }

    if (motion.canJump && input.jumpPressed && player.grounded && floorBelow) {
        player.velocityY = motion.jumpVelocity;
        player.grounded = false;
        step.jumped = true;
    }

    if (!player.grounded) {
        player.velocityY -= config_.gravity * dt;
        player.y += player.velocityY * dt;

        if (floorBelow && player.velocityY <= 0.0f &&
            player.y <= *groundBelow) {
            player.y = *groundBelow;
            player.velocityY = 0.0f;
            player.grounded = true;
            step.landed = true;
        }
    } else {
        player.y = groundBelow.value_or(environment.floorHeight);
        player.velocityY = 0.0f;
    }

    if (player.y < environment.voidResetHeight) {
        player.x = environment.spawnX;
        player.y = environment.spawnY;
        player.z = environment.spawnZ;
        player.velocityX = 0.0f;
        player.velocityY = 0.0f;
        player.velocityZ = 0.0f;
        player.grounded = true;
        ++player.voidRespawns;
        step.respawned = true;
    }

    if (step.sprinting && step.moved) {
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
