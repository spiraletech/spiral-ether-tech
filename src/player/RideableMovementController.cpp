#include "player/RideableMovementController.hpp"

#include <algorithm>
#include <cmath>

namespace hakui {

namespace {

constexpr float kMaximumDeltaSeconds = 0.10f;
constexpr float kMinimumTrickSpeed = 0.75f;
constexpr float kComboHoldSeconds = 2.75f;

RideDiscipline disciplineFor(LocomotionMode mode) noexcept
{
    switch (mode) {
        case LocomotionMode::Skateboard: return RideDiscipline::Skateboard;
        case LocomotionMode::BMX: return RideDiscipline::BMX;
        case LocomotionMode::OnFoot:
        case LocomotionMode::Car:
            return RideDiscipline::None;
    }
    return RideDiscipline::None;
}

float approach(float current, float target, float maximumDelta) noexcept
{
    if (current < target) {
        return std::min(current + maximumDelta, target);
    }
    return std::max(current - maximumDelta, target);
}

} // namespace

RideableFrame RideableMovementController::update(
    PlayerState& player,
    const RideableInput& input,
    const MovementEnvironment& environment,
    std::span<const WorldAffordanceVolume> affordances,
    float deltaSeconds
) noexcept
{
    RideableFrame frame;
    const float dt = std::clamp(
        std::isfinite(deltaSeconds) ? deltaSeconds : 0.0f,
        0.0f,
        kMaximumDeltaSeconds
    );
    const RideDiscipline discipline = disciplineFor(player.locomotion);
    if (discipline == RideDiscipline::None || dt <= 0.0f) {
        if (discipline == RideDiscipline::None) {
            reset();
        }
        return frame;
    }

    if (state_.discipline != discipline) {
        reset();
        state_.discipline = discipline;
        state_.phase = player.grounded ? RidePhase::Grounded : RidePhase::Airborne;
    }

    state_.phaseSeconds += dt;
    state_.comboWindowSeconds = std::max(0.0f, state_.comboWindowSeconds - dt);
    state_.steeringVisual = approach(
        state_.steeringVisual,
        std::clamp(input.movement.right, -1.0f, 1.0f),
        6.0f * dt
    );

    if (state_.phase == RidePhase::Crash) {
        player.velocityX = approach(player.velocityX, 0.0f, 18.0f * dt);
        player.velocityZ = approach(player.velocityZ, 0.0f, 18.0f * dt);
        frame.movement = movement_.update(
            player,
            {},
            environment,
            dt
        );
        if (state_.phaseSeconds >= 0.70f && player.grounded) {
            state_.phase = RidePhase::Grounded;
            state_.phaseSeconds = 0.0f;
            state_.activeTrick = RideTrick::None;
            state_.landingQuality = LandingQuality::None;
            state_.balance = 100.0f;
            state_.balanceOffset = 0.0f;
        }
        return frame;
    }

    MovementInput movementInput = input.movement;
    movementInput.jumpPressed = input.hopPressed &&
        state_.phase != RidePhase::Grinding &&
        state_.phase != RidePhase::Manual;
    const float preLandingVelocity = player.velocityY;
    frame.movement = movement_.update(
        player,
        movementInput,
        environment,
        dt
    );

    state_.speed = std::sqrt(
        player.velocityX * player.velocityX +
        player.velocityZ * player.velocityZ
    );
    const float referenceSpeed = discipline == RideDiscipline::Skateboard
        ? movement_.config().skateboardSprintSpeed
        : movement_.config().bmxSprintSpeed;
    state_.momentum = referenceSpeed > 0.0f
        ? std::clamp(state_.speed / referenceSpeed, 0.0f, 1.25f)
        : 0.0f;

    if (frame.movement.respawned) {
        const RideDiscipline retained = state_.discipline;
        reset();
        state_.discipline = retained;
        return frame;
    }

    const WorldAffordanceVolume* launch = nearby(
        WorldAffordance::Launch,
        player,
        affordances,
        0.45f
    );
    if (frame.movement.jumped) {
        if (launch) {
            player.velocityY += discipline == RideDiscipline::Skateboard
                ? 0.85f
                : 1.10f;
        }
        state_.phase = RidePhase::Airborne;
        state_.phaseSeconds = 0.0f;
        state_.airSeconds = 0.0f;
        state_.balance = 100.0f;
        state_.balanceOffset = 0.0f;
        state_.flipCommitted = false;
        beginTrick(
            discipline == RideDiscipline::Skateboard
                ? RideTrick::Ollie
                : RideTrick::BunnyHop,
            frame
        );
    }

    if (!player.grounded) {
        state_.phase = RidePhase::Airborne;
        state_.airSeconds += dt;
        const RideTrick airTrick = discipline == RideDiscipline::Skateboard
            ? (input.flipLeftPressed
                ? RideTrick::Kickflip
                : (input.flipRightPressed ? RideTrick::Heelflip : RideTrick::None))
            : ((input.flipLeftPressed || input.flipRightPressed)
                ? RideTrick::BmxTabletop
                : RideTrick::None);
        if (airTrick != RideTrick::None && !state_.flipCommitted) {
            state_.flipCommitted = true;
            beginTrick(airTrick, frame);
        }

        const WorldAffordanceVolume* grind = nearby(
            WorldAffordance::Grindable,
            player,
            affordances,
            0.55f
        );
        if (input.grindHeld && grind && state_.speed >= kMinimumTrickSpeed &&
            player.velocityY <= 1.0f) {
            state_.phase = RidePhase::Grinding;
            state_.phaseSeconds = 0.0f;
            state_.activeAffordanceId = grind->id;
            state_.balance = 100.0f;
            state_.balanceOffset = 0.0f;
            player.grounded = true;
            player.velocityY = 0.0f;
            player.y = grind->primaryAnchor.y;
            beginTrick(
                discipline == RideDiscipline::Skateboard
                    ? RideTrick::BoardGrind
                    : RideTrick::PegGrind,
                frame
            );
            frame.grindStarted = true;
        }
    }

    if (player.grounded && state_.phase != RidePhase::Grinding &&
        state_.phase != RidePhase::Crash && input.grindHeld &&
        state_.speed >= kMinimumTrickSpeed) {
        const WorldAffordanceVolume* grind = nearby(
            WorldAffordance::Grindable,
            player,
            affordances,
            0.45f
        );
        if (grind) {
            state_.phase = RidePhase::Grinding;
            state_.phaseSeconds = 0.0f;
            state_.activeAffordanceId = grind->id;
            state_.balance = 100.0f;
            state_.balanceOffset = 0.0f;
            player.y = grind->primaryAnchor.y;
            player.velocityY = 0.0f;
            beginTrick(
                discipline == RideDiscipline::Skateboard
                    ? RideTrick::BoardGrind
                    : RideTrick::PegGrind,
                frame
            );
            frame.grindStarted = true;
        }
    }

    if (state_.phase == RidePhase::Grinding) {
        const WorldAffordanceVolume* grind = nearby(
            WorldAffordance::Grindable,
            player,
            affordances,
            0.65f
        );
        if (!input.grindHeld || !grind || state_.speed < 0.35f) {
            state_.phase = player.grounded ? RidePhase::Grounded : RidePhase::Airborne;
            state_.phaseSeconds = 0.0f;
            state_.activeAffordanceId = 0;
        } else {
            state_.activeAffordanceId = grind->id;
            updateBalance(
                input.movement.right,
                discipline == RideDiscipline::Skateboard ? 0.30f : -0.24f,
                dt
            );
            const float xSpan = grind->maximumX - grind->minimumX;
            const float zSpan = grind->maximumZ - grind->minimumZ;
            if (xSpan >= zSpan) {
                player.z = grind->primaryAnchor.z;
            } else {
                player.x = grind->primaryAnchor.x;
            }
            player.y = grind->primaryAnchor.y;
            player.grounded = true;
            player.velocityY = 0.0f;
            if (state_.balance <= 2.0f) {
                beginBail(player, frame);
            }
        }
    } else if (player.grounded && !frame.movement.landed) {
        const WorldAffordanceVolume* manual = nearby(
            WorldAffordance::ManualZone,
            player,
            affordances,
            0.25f
        );
        if (input.manualHeld && manual && state_.speed >= kMinimumTrickSpeed) {
            if (state_.phase != RidePhase::Manual) {
                state_.phase = RidePhase::Manual;
                state_.phaseSeconds = 0.0f;
                state_.activeAffordanceId = manual->id;
                state_.balance = 100.0f;
                state_.balanceOffset = 0.0f;
                beginTrick(
                    discipline == RideDiscipline::Skateboard
                        ? RideTrick::BoardManual
                        : RideTrick::WheelManual,
                    frame
                );
                frame.manualStarted = true;
            }
            updateBalance(
                input.movement.forward,
                discipline == RideDiscipline::Skateboard ? -0.26f : 0.34f,
                dt
            );
            if (state_.balance <= 2.0f) {
                beginBail(player, frame);
            }
        } else if (state_.phase == RidePhase::Manual) {
            state_.phase = RidePhase::Grounded;
            state_.phaseSeconds = 0.0f;
            state_.activeAffordanceId = 0;
            state_.balance = 100.0f;
            state_.balanceOffset = 0.0f;
        }
    }

    if (frame.movement.landed && state_.phase != RidePhase::Grinding) {
        const float impactSpeed = std::fabs(preLandingVelocity);
        if (impactSpeed < 7.25f && state_.balance >= 72.0f) {
            state_.landingQuality = LandingQuality::Perfect;
        } else if (impactSpeed < 9.75f && state_.balance >= 42.0f) {
            state_.landingQuality = LandingQuality::Clean;
        } else if (impactSpeed < 12.50f) {
            state_.landingQuality = LandingQuality::Sketchy;
        } else {
            beginBail(player, frame);
            return frame;
        }
        state_.phase = RidePhase::Landing;
        state_.phaseSeconds = 0.0f;
        state_.activeAffordanceId = 0;
        state_.activeTrick = RideTrick::Land;
        appendCombo(RideTrick::Land);
        frame.landed = true;
    }

    if (state_.phase == RidePhase::Landing && state_.phaseSeconds >= 0.38f) {
        state_.phase = RidePhase::Grounded;
        state_.phaseSeconds = 0.0f;
    }
    if (state_.phase == RidePhase::Grounded && state_.comboWindowSeconds <= 0.0f) {
        state_.comboCount = 0;
        state_.activeTrick = RideTrick::None;
        state_.landingQuality = LandingQuality::None;
    }
    return frame;
}

void RideableMovementController::reset() noexcept
{
    state_ = {};
}

const RideableState& RideableMovementController::state() const noexcept
{
    return state_;
}

std::string_view RideableMovementController::phaseLabel(RidePhase phase) noexcept
{
    switch (phase) {
        case RidePhase::Grounded: return "GROUND";
        case RidePhase::Airborne: return "AIR";
        case RidePhase::Grinding: return "GRIND";
        case RidePhase::Manual: return "MANUAL";
        case RidePhase::Landing: return "LANDING";
        case RidePhase::Crash: return "BAIL";
    }
    return "GROUND";
}

std::string_view RideableMovementController::trickLabel(RideTrick trick) noexcept
{
    switch (trick) {
        case RideTrick::None: return "READY";
        case RideTrick::Ollie: return "OLLIE";
        case RideTrick::BunnyHop: return "BUNNY HOP";
        case RideTrick::Kickflip: return "KICKFLIP";
        case RideTrick::Heelflip: return "HEELFLIP";
        case RideTrick::BmxTabletop: return "TABLETOP";
        case RideTrick::BoardGrind: return "BOARD GRIND";
        case RideTrick::PegGrind: return "PEG GRIND";
        case RideTrick::BoardManual: return "BOARD MANUAL";
        case RideTrick::WheelManual: return "WHEEL MANUAL";
        case RideTrick::Land: return "LAND";
        case RideTrick::Bail: return "BAIL";
    }
    return "READY";
}

std::string_view RideableMovementController::landingLabel(
    LandingQuality quality
) noexcept
{
    switch (quality) {
        case LandingQuality::None: return "--";
        case LandingQuality::Sketchy: return "SKETCHY";
        case LandingQuality::Clean: return "CLEAN";
        case LandingQuality::Perfect: return "PERFECT";
        case LandingQuality::Bail: return "BAIL";
    }
    return "--";
}

const WorldAffordanceVolume* RideableMovementController::nearby(
    WorldAffordance affordance,
    const PlayerState& player,
    std::span<const WorldAffordanceVolume> volumes,
    float padding
) const noexcept
{
    for (const WorldAffordanceVolume& volume : volumes) {
        if (!hasAffordance(volume.affordances, affordance)) {
            continue;
        }
        if (player.x >= volume.minimumX - padding &&
            player.x <= volume.maximumX + padding &&
            player.y >= volume.minimumY - padding - 0.8f &&
            player.y <= volume.maximumY + padding + 0.8f &&
            player.z >= volume.minimumZ - padding &&
            player.z <= volume.maximumZ + padding) {
            return &volume;
        }
    }
    return nullptr;
}

void RideableMovementController::beginTrick(
    RideTrick trick,
    RideableFrame& frame
) noexcept
{
    state_.activeTrick = trick;
    appendCombo(trick);
    frame.trickStarted = true;
}

void RideableMovementController::appendCombo(RideTrick trick) noexcept
{
    if (trick == RideTrick::None) {
        return;
    }
    if (state_.comboCount == state_.combo.size()) {
        for (std::size_t index = 1; index < state_.combo.size(); ++index) {
            state_.combo[index - 1] = state_.combo[index];
        }
        --state_.comboCount;
    }
    state_.combo[state_.comboCount++] = trick;
    state_.comboWindowSeconds = kComboHoldSeconds;
}

void RideableMovementController::beginBail(
    PlayerState& player,
    RideableFrame& frame
) noexcept
{
    state_.phase = RidePhase::Crash;
    state_.phaseSeconds = 0.0f;
    state_.activeTrick = RideTrick::Bail;
    state_.landingQuality = LandingQuality::Bail;
    state_.activeAffordanceId = 0;
    state_.balance = 0.0f;
    appendCombo(RideTrick::Bail);
    player.velocityX *= 0.22f;
    player.velocityZ *= 0.22f;
    frame.bailed = true;
}

void RideableMovementController::updateBalance(
    float correction,
    float naturalDrift,
    float deltaSeconds
) noexcept
{
    state_.balanceOffset += naturalDrift * deltaSeconds;
    state_.balanceOffset -= std::clamp(correction, -1.0f, 1.0f) *
        0.92f * deltaSeconds;
    state_.balanceOffset = std::clamp(state_.balanceOffset, -1.25f, 1.25f);
    state_.balance = std::clamp(
        100.0f - std::fabs(state_.balanceOffset) * 92.0f,
        0.0f,
        100.0f
    );
}

} // namespace hakui
