#include "player/RideableMovementController.hpp"

#include <algorithm>
#include <cmath>

namespace hakui {

namespace {

constexpr float kMaximumDeltaSeconds = 0.10f;
constexpr float kMinimumTrickSpeed = 0.75f;
constexpr float kComboHoldSeconds = 2.75f;
constexpr float kMinimumGrindAlignment = 0.42f;
constexpr float kPi = 3.14159265358979323846f;

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

float length(const RideRotation& value) noexcept
{
    return std::sqrt(
        value.x * value.x + value.y * value.y + value.z * value.z
    );
}

RideRotation scaled(const RideRotation& value, float factor) noexcept
{
    return {value.x * factor, value.y * factor, value.z * factor};
}

RideRotation difference(
    const RideRotation& lhs,
    const RideRotation& rhs
) noexcept
{
    return {lhs.x - rhs.x, lhs.y - rhs.y, lhs.z - rhs.z};
}

RideRotation surfaceNormalAt(
    const MovementEnvironment& environment,
    float x,
    float z
) noexcept
{
    for (const WalkableSurface& surface : environment.surfaces) {
        if (x < surface.minimumX || x > surface.maximumX ||
            z < surface.minimumZ || z > surface.maximumZ) {
            continue;
        }
        const float inverseLength = 1.0f / std::max(
            0.0001f,
            std::sqrt(
                surface.slopeX * surface.slopeX +
                surface.slopeZ * surface.slopeZ + 1.0f
            )
        );
        return {
            -surface.slopeX * inverseLength,
            inverseLength,
            -surface.slopeZ * inverseLength
        };
    }
    return {0.0f, 1.0f, 0.0f};
}

} // namespace

RideableMovementController::RideableMovementController(
    RidePhysicsTuning tuning
) : tuning_(tuning)
{
    tuning_.skateboardPopImpulseMin = std::max(
        0.0f,
        tuning_.skateboardPopImpulseMin
    );
    tuning_.skateboardPopImpulseMax = std::max(
        tuning_.skateboardPopImpulseMin,
        tuning_.skateboardPopImpulseMax
    );
    tuning_.bmxPopImpulseMin = std::max(0.0f, tuning_.bmxPopImpulseMin);
    tuning_.bmxPopImpulseMax = std::max(
        tuning_.bmxPopImpulseMin,
        tuning_.bmxPopImpulseMax
    );
    tuning_.cleanCompletion = std::clamp(tuning_.cleanCompletion, 0.5f, 1.0f);
    tuning_.sketchyCompletion = std::clamp(
        tuning_.sketchyCompletion,
        0.25f,
        tuning_.cleanCompletion
    );
    tuning_.cleanImpactSpeed = std::max(0.0f, tuning_.cleanImpactSpeed);
    tuning_.sketchyImpactSpeed = std::max(
        tuning_.cleanImpactSpeed,
        tuning_.sketchyImpactSpeed
    );
    tuning_.cleanAngularSpeed = std::max(0.0f, tuning_.cleanAngularSpeed);
    tuning_.sketchyAngularSpeed = std::max(
        tuning_.cleanAngularSpeed,
        tuning_.sketchyAngularSpeed
    );
}

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
    if (player.grounded && state_.phase != RidePhase::Landing) {
        state_.popPreload = std::clamp(input.popPreload, 0.0f, 1.0f);
    }
    state_.steeringVisual = approach(
        state_.steeringVisual,
        std::clamp(input.movement.right, -1.0f, 1.0f),
        6.0f * dt
    );
    state_.propulsion = approach(
        state_.propulsion,
        std::clamp(input.propulsion, 0.0f, 1.0f),
        5.0f * dt
    );

    if (state_.phase == RidePhase::Crash) {
        player.velocityX = approach(player.velocityX, 0.0f, 8.0f * dt);
        player.velocityZ = approach(player.velocityZ, 0.0f, 8.0f * dt);
        state_.tumbleRadians += (5.4f + length(state_.angularVelocity) * 0.18f) * dt;
        state_.rideSeparation += state_.rideSeparationVelocity * dt;
        state_.rideSeparationVelocity = approach(
            state_.rideSeparationVelocity,
            0.0f,
            2.8f * dt
        );
        state_.rideableRotation.x += state_.angularVelocity.x * dt;
        state_.rideableRotation.y += state_.angularVelocity.y * dt;
        state_.rideableRotation.z += state_.angularVelocity.z * dt;
        state_.angularVelocity = scaled(
            state_.angularVelocity,
            std::max(0.0f, 1.0f - 2.2f * dt)
        );
        frame.movement = movement_.update(player, {}, environment, dt);
        if (state_.phaseSeconds >= 0.90f && player.grounded) {
            const RideDiscipline retained = state_.discipline;
            const float retainedSpeed = state_.speed;
            reset();
            state_.discipline = retained;
            state_.speed = retainedSpeed;
            state_.phase = RidePhase::Grounded;
        }
        updateBodyMechanics(player);
        return frame;
    }

    const bool wasAirborne = !player.grounded;
    MovementInput movementInput = input.movement;
    movementInput.jumpPressed = input.popPressed &&
        state_.phase != RidePhase::Grinding &&
        state_.phase != RidePhase::Manual;
    const float preLandingVelocity = player.velocityY;
    frame.movement = movement_.update(player, movementInput, environment, dt);

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
        updateBodyMechanics(player);
        return frame;
    }

    const WorldAffordanceVolume* launch = nearby(
        WorldAffordance::Launch,
        player,
        affordances,
        0.45f
    );
    if (frame.movement.jumped) {
        const float preload = std::clamp(input.popPreload, 0.0f, 1.0f);
        const float minimumImpulse = discipline == RideDiscipline::Skateboard
            ? tuning_.skateboardPopImpulseMin
            : tuning_.bmxPopImpulseMin;
        const float maximumImpulse = discipline == RideDiscipline::Skateboard
            ? tuning_.skateboardPopImpulseMax
            : tuning_.bmxPopImpulseMax;
        state_.popPreload = preload;
        state_.popImpulse = minimumImpulse +
            (maximumImpulse - minimumImpulse) * preload;
        if (launch) {
            state_.popImpulse += discipline == RideDiscipline::Skateboard
                ? 0.85f
                : 1.10f;
        }
        player.velocityY = std::max(player.velocityY, state_.popImpulse);
        state_.phase = RidePhase::Airborne;
        state_.phaseSeconds = 0.0f;
        state_.airSeconds = 0.0f;
        state_.trickSeconds = 0.0f;
        state_.balance = 100.0f;
        state_.balanceOffset = 0.0f;
        state_.flipCommitted = false;
        state_.bailReason = BailReason::None;
        state_.landingQuality = LandingQuality::None;
        state_.rideableRotation = {};
        state_.angularVelocity = {};
        state_.targetRotation = {};
        state_.rotationCompletion = 1.0f;
        state_.rotationTravel = 0.0f;
        state_.rotationTravelTarget = 0.0f;
        state_.rotationChannel = RideRotationChannel::None;
        state_.leftHandGripError = 0.0f;
        state_.rightHandGripError = 0.0f;
        state_.leftFootAnchorError = 0.0f;
        state_.rightFootAnchorError = 0.0f;
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
        RideTrick airTrick = input.trick.valid
            ? trickFor(discipline, input.trick.direction)
            : RideTrick::None;
        if (airTrick == RideTrick::None && input.stylePressed) {
            airTrick = discipline == RideDiscipline::Skateboard
                ? RideTrick::BoardGrab
                : RideTrick::BmxTabletop;
        }
        if (airTrick != RideTrick::None && !state_.flipCommitted) {
            state_.flipCommitted = true;
            state_.lastTrickDirection = input.trick.direction;
            beginPhysicalTrick(airTrick, frame);
        }

        const float spinIntent = static_cast<float>(input.spinRight) -
            static_cast<float>(input.spinLeft);
        const float spinResponse = discipline == RideDiscipline::Skateboard
            ? 7.4f
            : 5.8f;
        state_.spinVelocity = approach(
            state_.spinVelocity,
            spinIntent * spinResponse,
            13.0f * dt
        );
        state_.bodySpinRadians += state_.spinVelocity * dt;
        player.yaw += state_.spinVelocity * dt;

        updatePhysicalRotation(dt);
        if (state_.flipCommitted) {
            const float separation = std::sin(
                std::clamp(state_.rotationCompletion, 0.0f, 1.0f) * kPi
            );
            if (discipline == RideDiscipline::Skateboard) {
                state_.leftFootAnchorError = std::max(0.0f, separation) * 0.24f;
                state_.rightFootAnchorError = std::max(0.0f, separation) * 0.20f;
            } else if (state_.rotationChannel == RideRotationChannel::BmxFrame ||
                       state_.rotationChannel == RideRotationChannel::BmxCrank) {
                state_.leftFootAnchorError = std::max(0.0f, separation) * 0.28f;
                state_.rightFootAnchorError = std::max(0.0f, separation) * 0.28f;
            }
            state_.footContactAlignment = std::clamp(
                state_.rotationCompletion,
                0.0f,
                1.0f
            );
        }

        const WorldAffordanceVolume* grind = nearby(
            WorldAffordance::Grindable,
            player,
            affordances,
            0.55f
        );
        if (input.grindHeld && grind && validGrindApproach(*grind, player) &&
            player.velocityY <= 1.0f) {
            state_.phase = RidePhase::Grinding;
            state_.phaseSeconds = 0.0f;
            state_.activeAffordanceId = grind->id;
            state_.activeGrindAttachment = discipline == RideDiscipline::Skateboard
                ? RideGrindAttachment::BoardTrucks
                : RideGrindAttachment::BmxPegs;
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
    } else if (wasAirborne && state_.phase == RidePhase::Airborne) {
        // Integrate the final deterministic slice before ground-contact
        // evaluation; an unfinished rotation remains unfinished.
        updatePhysicalRotation(dt);
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
        if (grind && validGrindApproach(*grind, player)) {
            state_.phase = RidePhase::Grinding;
            state_.phaseSeconds = 0.0f;
            state_.activeAffordanceId = grind->id;
            state_.activeGrindAttachment = discipline == RideDiscipline::Skateboard
                ? RideGrindAttachment::BoardTrucks
                : RideGrindAttachment::BmxPegs;
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
            state_.activeGrindAttachment = RideGrindAttachment::None;
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
                beginBail(player, frame, BailReason::LostBalance);
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
                beginBail(player, frame, BailReason::LostBalance);
            }
        } else if (state_.phase == RidePhase::Manual) {
            state_.phase = RidePhase::Grounded;
            state_.phaseSeconds = 0.0f;
            state_.activeAffordanceId = 0;
            state_.balance = 100.0f;
            state_.balanceOffset = 0.0f;
        }
    }

    if (frame.movement.landed && state_.phase != RidePhase::Grinding &&
        state_.phase != RidePhase::Crash) {
        state_.surfaceNormal = surfaceNormalAt(environment, player.x, player.z);
        BailReason reason = BailReason::None;
        const LandingQuality quality = evaluateLanding(
            std::fabs(preLandingVelocity),
            state_.speed,
            state_.surfaceNormal,
            reason
        );
        frame.evaluatedLanding = quality;
        if (quality == LandingQuality::Failed || quality == LandingQuality::Bail) {
            beginBail(player, frame, reason, quality);
            updateBodyMechanics(player);
            return frame;
        }
        state_.landingQuality = quality;
        state_.bailReason = BailReason::None;
        state_.phase = RidePhase::Landing;
        state_.phaseSeconds = 0.0f;
        state_.activeAffordanceId = 0;
        state_.activeGrindAttachment = RideGrindAttachment::None;
        state_.activeTrick = RideTrick::Land;
        state_.leftFootAnchorError = quality == LandingQuality::Sketchy ? 0.08f : 0.0f;
        state_.rightFootAnchorError = quality == LandingQuality::Sketchy ? 0.06f : 0.0f;
        appendCombo(RideTrick::Land);
        frame.landed = true;
    }

    if (state_.phase == RidePhase::Landing && state_.phaseSeconds >= 0.38f) {
        state_.phase = RidePhase::Grounded;
        state_.phaseSeconds = 0.0f;
        state_.rideableRotation = {};
        state_.angularVelocity = {};
        state_.targetRotation = {};
        state_.rotationCompletion = 1.0f;
        state_.rotationChannel = RideRotationChannel::None;
        state_.footContactAlignment = 1.0f;
        state_.leftFootAnchorError = 0.0f;
        state_.rightFootAnchorError = 0.0f;
    }
    if (player.grounded && state_.phase != RidePhase::Grinding) {
        state_.spinVelocity = approach(state_.spinVelocity, 0.0f, 18.0f * dt);
    }
    if (state_.phase == RidePhase::Grounded && state_.comboWindowSeconds <= 0.0f) {
        state_.comboCount = 0;
        state_.activeTrick = RideTrick::None;
        state_.landingQuality = LandingQuality::None;
        state_.bailReason = BailReason::None;
    }
    updateBodyMechanics(player);
    return frame;
}

void RideableMovementController::reset() noexcept
{
    state_ = {};
    state_.body.skateStance = skateStance_;
}

void RideableMovementController::setSkateStance(SkateStance stance) noexcept
{
    skateStance_ = stance;
    state_.body.skateStance = stance;
}

const RideableState& RideableMovementController::state() const noexcept
{
    return state_;
}

const RidePhysicsTuning& RideableMovementController::tuning() const noexcept
{
    return tuning_;
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
        case RideTrick::PopShoveIt: return "POP SHOVE-IT";
        case RideTrick::Impossible: return "IMPOSSIBLE";
        case RideTrick::VarialFlip: return "VARIAL FLIP";
        case RideTrick::BoardGrab: return "BOARD GRAB";
        case RideTrick::BmxTabletop: return "TABLETOP";
        case RideTrick::BmxTailwhipLeft: return "TAILWHIP LEFT";
        case RideTrick::BmxTailwhipRight: return "TAILWHIP RIGHT";
        case RideTrick::BmxBarspin: return "BARSPIN";
        case RideTrick::BmxCrankflip: return "CRANKFLIP";
        case RideTrick::BmxXUp: return "X-UP";
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
        case LandingQuality::Clean: return "CLEAN";
        case LandingQuality::Sketchy: return "SKETCHY";
        case LandingQuality::Failed: return "FAILED";
        case LandingQuality::Bail: return "BAIL";
    }
    return "--";
}

std::string_view RideableMovementController::bailReasonLabel(
    BailReason reason
) noexcept
{
    switch (reason) {
        case BailReason::None: return "NONE";
        case BailReason::UnderRotated: return "UNDER_ROTATED";
        case BailReason::InvertedRideable: return "INVERTED_RIDEABLE";
        case BailReason::ExcessiveImpact: return "EXCESSIVE_IMPACT";
        case BailReason::ExcessiveAngularVelocity: return "EXCESSIVE_ANGULAR_VELOCITY";
        case BailReason::ContactMisalignment: return "CONTACT_MISALIGNMENT";
        case BailReason::LostBalance: return "LOST_BALANCE";
    }
    return "NONE";
}

std::string_view RideableMovementController::rotationChannelLabel(
    RideRotationChannel channel
) noexcept
{
    switch (channel) {
        case RideRotationChannel::None: return "NONE";
        case RideRotationChannel::Rideable: return "RIDEABLE";
        case RideRotationChannel::BoardDeck: return "BOARD_DECK";
        case RideRotationChannel::BmxFrame: return "BMX_FRAME";
        case RideRotationChannel::BmxSteering: return "BMX_STEERING";
        case RideRotationChannel::BmxCrank: return "BMX_CRANK";
    }
    return "NONE";
}

std::string_view RideableMovementController::skateStanceLabel(
    SkateStance stance
) noexcept
{
    switch (stance) {
        case SkateStance::Regular: return "REGULAR";
        case SkateStance::Goofy: return "GOOFY";
    }
    return "REGULAR";
}

std::string_view RideableMovementController::footContactLabel(
    RideFootContactState state
) noexcept
{
    switch (state) {
        case RideFootContactState::Anchored: return "ANCHORED";
        case RideFootContactState::ReleasedForTrick: return "RELEASED_FOR_TRICK";
        case RideFootContactState::Reacquiring: return "REACQUIRING";
        case RideFootContactState::Landed: return "LANDED";
    }
    return "ANCHORED";
}

std::string_view RideableMovementController::airPoseLabel(
    RideAirPose pose
) noexcept
{
    switch (pose) {
        case RideAirPose::None: return "NONE";
        case RideAirPose::OlliePop: return "OLLIE_POP";
        case RideAirPose::OllieRise: return "OLLIE_RISE";
        case RideAirPose::OllieLevel: return "OLLIE_LEVEL";
        case RideAirPose::OllieDescent: return "OLLIE_DESCENT";
        case RideAirPose::Kickflip: return "KICKFLIP";
        case RideAirPose::Heelflip: return "HEELFLIP";
        case RideAirPose::PopShoveIt: return "POP_SHOVE_IT";
        case RideAirPose::Impossible: return "IMPOSSIBLE";
        case RideAirPose::VarialFlip: return "VARIAL_FLIP";
        case RideAirPose::BoardGrab: return "BOARD_GRAB";
        case RideAirPose::BmxPull: return "BMX_PULL";
        case RideAirPose::BmxTuck: return "BMX_TUCK";
        case RideAirPose::BmxDescent: return "BMX_DESCENT";
        case RideAirPose::BmxTrick: return "BMX_TRICK";
        case RideAirPose::Bail: return "BAIL";
    }
    return "NONE";
}

void RideableMovementController::updateBodyMechanics(
    const PlayerState& player
) noexcept
{
    RideBodyMechanicsState body;
    body.skateStance = skateStance_;
    body.frontFootAnchorError = skateStance_ == SkateStance::Regular
        ? state_.leftFootAnchorError
        : state_.rightFootAnchorError;
    body.rearFootAnchorError = skateStance_ == SkateStance::Regular
        ? state_.rightFootAnchorError
        : state_.leftFootAnchorError;

    if (state_.phase == RidePhase::Crash) {
        body.footContact = RideFootContactState::ReleasedForTrick;
        body.airPose = RideAirPose::Bail;
        body.leftKneeFlex = 0.12f;
        body.rightKneeFlex = 0.92f;
        body.leftElbowFlex = 0.18f;
        body.rightElbowFlex = 0.86f;
        body.armCounterbalance = 1.0f;
        body.torsoLean = 0.58f;
        state_.body = body;
        return;
    }

    const float preload = player.grounded && state_.phase != RidePhase::Landing
        ? std::clamp(state_.popPreload, 0.0f, 1.0f)
        : 0.0f;
    body.preloadPoseWeight = preload;

    if (state_.discipline == RideDiscipline::Skateboard) {
        const float stanceDirection = skateStance_ == SkateStance::Regular
            ? 1.0f
            : -1.0f;
        // The pelvis commits to the sideways board stance. The chest opens
        // toward travel and the head nearly cancels the remaining yaw, so the
        // avatar is articulated rather than rotated as one rigid mannequin.
        body.pelvisYawRelativeToBoard = stanceDirection * 1.40f;
        body.torsoYawRelativeToBoard = stanceDirection * 0.70f;
        body.headYawRelativeToBoard = stanceDirection * 0.12f;
        body.leftKneeFlex = 0.44f + preload * 0.68f;
        body.rightKneeFlex = 0.52f + preload * 0.82f;
        body.leftElbowFlex = 0.24f;
        body.rightElbowFlex = 0.30f;
        body.torsoLean = 0.04f + preload * 0.24f;
        body.armCounterbalance = state_.steeringVisual * 0.20f;

        if (state_.phase == RidePhase::Manual) {
            body.leftKneeFlex += 0.18f;
            body.rightKneeFlex += 0.34f;
            body.torsoLean = -0.16f;
            body.armCounterbalance = state_.balanceOffset * 0.85f;
        } else if (state_.phase == RidePhase::Grinding) {
            body.leftKneeFlex += 0.24f;
            body.rightKneeFlex += 0.20f;
            body.torsoLean = 0.10f;
            body.armCounterbalance = state_.balanceOffset * 1.05f;
        } else if (state_.phase == RidePhase::Airborne) {
            const float airProgress = std::clamp(
                state_.airSeconds / std::max(0.55f, state_.minimumTrickAirtime),
                0.0f,
                1.0f
            );
            if (state_.flipCommitted) {
                if (state_.rotationCompletion < 0.66f) {
                    body.footContact = RideFootContactState::ReleasedForTrick;
                } else {
                    body.footContact = RideFootContactState::Reacquiring;
                }
                switch (state_.activeTrick) {
                    case RideTrick::Kickflip:
                        body.airPose = RideAirPose::Kickflip;
                        body.leftKneeFlex = 1.02f;
                        body.rightKneeFlex = 0.66f;
                        body.frontFootLift = 0.24f;
                        body.armCounterbalance = -0.72f;
                        break;
                    case RideTrick::Heelflip:
                        body.airPose = RideAirPose::Heelflip;
                        body.leftKneeFlex = 0.66f;
                        body.rightKneeFlex = 1.04f;
                        body.frontFootLift = 0.16f;
                        body.armCounterbalance = 0.76f;
                        break;
                    case RideTrick::PopShoveIt:
                        body.airPose = RideAirPose::PopShoveIt;
                        body.leftKneeFlex = 0.78f;
                        body.rightKneeFlex = 0.74f;
                        body.rearLegDrive = 0.30f;
                        body.torsoYawRelativeToBoard += stanceDirection * 0.24f;
                        break;
                    case RideTrick::Impossible:
                        body.airPose = RideAirPose::Impossible;
                        body.leftKneeFlex = 1.12f;
                        body.rightKneeFlex = 0.58f;
                        body.frontFootLift = 0.34f;
                        body.rearLegDrive = 0.38f;
                        body.armCounterbalance = -0.42f;
                        break;
                    case RideTrick::VarialFlip:
                        body.airPose = RideAirPose::VarialFlip;
                        body.leftKneeFlex = 1.00f;
                        body.rightKneeFlex = 0.88f;
                        body.frontFootLift = 0.28f;
                        body.rearLegDrive = 0.26f;
                        body.armCounterbalance = 0.54f;
                        break;
                    case RideTrick::BoardGrab:
                        body.airPose = RideAirPose::BoardGrab;
                        body.leftKneeFlex = 1.18f;
                        body.rightKneeFlex = 1.08f;
                        body.torsoLean = 0.34f;
                        break;
                    default:
                        body.airPose = RideAirPose::OllieLevel;
                        break;
                }
            } else {
                body.footContact = airProgress < 0.22f
                    ? RideFootContactState::ReleasedForTrick
                    : RideFootContactState::Reacquiring;
                if (state_.airSeconds < 0.09f) {
                    body.airPose = RideAirPose::OlliePop;
                    body.rearLegDrive = 0.42f;
                    body.leftKneeFlex = 0.70f;
                    body.rightKneeFlex = 1.04f;
                } else if (player.velocityY > 1.0f) {
                    body.airPose = RideAirPose::OllieRise;
                    body.frontFootLift = 0.30f;
                    body.leftKneeFlex = 1.02f;
                    body.rightKneeFlex = 0.76f;
                } else if (player.velocityY > -1.2f) {
                    body.airPose = RideAirPose::OllieLevel;
                    body.frontFootLift = 0.10f;
                    body.leftKneeFlex = 0.84f;
                    body.rightKneeFlex = 0.84f;
                } else {
                    body.airPose = RideAirPose::OllieDescent;
                    body.leftKneeFlex = 0.70f;
                    body.rightKneeFlex = 0.74f;
                }
            }
        } else if (state_.phase == RidePhase::Landing) {
            const float landingProgress = std::clamp(
                state_.phaseSeconds / 0.38f,
                0.0f,
                1.0f
            );
            const float qualityWeight = state_.landingQuality == LandingQuality::Sketchy
                ? 0.92f
                : 0.58f;
            body.landingCompression = std::sin(landingProgress * kPi) * qualityWeight;
            body.footContact = RideFootContactState::Landed;
            body.leftKneeFlex += body.landingCompression *
                (state_.landingQuality == LandingQuality::Sketchy ? 1.00f : 0.74f);
            body.rightKneeFlex += body.landingCompression *
                (state_.landingQuality == LandingQuality::Sketchy ? 0.72f : 0.74f);
            body.armCounterbalance = state_.landingQuality == LandingQuality::Sketchy
                ? 0.62f
                : 0.18f;
        }
    } else if (state_.discipline == RideDiscipline::BMX) {
        body.pelvisYawRelativeToBoard = state_.steeringVisual * 0.05f;
        body.torsoYawRelativeToBoard = state_.steeringVisual * 0.12f;
        body.headYawRelativeToBoard = 0.0f;
        body.leftKneeFlex = 0.66f + preload * 0.54f;
        body.rightKneeFlex = 0.72f + preload * 0.54f;
        body.leftElbowFlex = 0.70f + preload * 0.22f;
        body.rightElbowFlex = 0.70f + preload * 0.22f;
        body.torsoLean = 0.24f + preload * 0.18f;
        body.armCounterbalance = state_.steeringVisual * 0.34f;

        if (state_.phase == RidePhase::Manual) {
            body.leftKneeFlex += 0.24f;
            body.rightKneeFlex += 0.24f;
            body.leftElbowFlex = 0.42f;
            body.rightElbowFlex = 0.42f;
            body.torsoLean = -0.08f;
        } else if (state_.phase == RidePhase::Grinding) {
            body.leftKneeFlex += 0.18f;
            body.rightKneeFlex += 0.18f;
            body.torsoLean = 0.30f;
            body.armCounterbalance = state_.balanceOffset * 0.82f;
        } else if (state_.phase == RidePhase::Airborne) {
            body.footContact = state_.flipCommitted &&
                state_.rotationCompletion < 0.68f
                ? RideFootContactState::ReleasedForTrick
                : RideFootContactState::Reacquiring;
            if (state_.flipCommitted) {
                body.airPose = RideAirPose::BmxTrick;
                body.leftKneeFlex = 0.96f;
                body.rightKneeFlex = 0.96f;
                body.leftElbowFlex = 0.54f;
                body.rightElbowFlex = 0.54f;
            } else if (state_.airSeconds < 0.13f || player.velocityY > 2.2f) {
                body.airPose = RideAirPose::BmxPull;
                body.leftKneeFlex = 0.84f;
                body.rightKneeFlex = 0.90f;
                body.leftElbowFlex = 0.34f;
                body.rightElbowFlex = 0.34f;
                body.torsoLean = 0.08f;
            } else if (player.velocityY > -1.2f) {
                body.airPose = RideAirPose::BmxTuck;
                body.leftKneeFlex = 1.12f;
                body.rightKneeFlex = 1.12f;
                body.leftElbowFlex = 0.64f;
                body.rightElbowFlex = 0.64f;
            } else {
                body.airPose = RideAirPose::BmxDescent;
                body.leftKneeFlex = 0.78f;
                body.rightKneeFlex = 0.82f;
                body.leftElbowFlex = 0.76f;
                body.rightElbowFlex = 0.76f;
            }
        } else if (state_.phase == RidePhase::Landing) {
            const float landingProgress = std::clamp(
                state_.phaseSeconds / 0.38f,
                0.0f,
                1.0f
            );
            const float qualityWeight = state_.landingQuality == LandingQuality::Sketchy
                ? 0.88f
                : 0.54f;
            body.landingCompression = std::sin(landingProgress * kPi) * qualityWeight;
            body.footContact = RideFootContactState::Landed;
            body.leftKneeFlex += body.landingCompression * 0.78f;
            body.rightKneeFlex += body.landingCompression * 0.96f;
            body.leftElbowFlex += body.landingCompression * 0.24f;
            body.rightElbowFlex += body.landingCompression * 0.24f;
        }
    }

    state_.body = body;
}

TrickPhysicalIntent RideableMovementController::physicalIntentFor(
    RideTrick trick
) noexcept
{
    switch (trick) {
        case RideTrick::Kickflip:
            return {RideRotationChannel::BoardDeck, {0.0f, 0.0f, 1.0f},
                    1.0f, 2.0f * kPi, 12.6f, 0.42f, 0.08f, false};
        case RideTrick::Heelflip:
            return {RideRotationChannel::BoardDeck, {0.0f, 0.0f, 1.0f},
                    -1.0f, 2.0f * kPi, 12.2f, 0.43f, -0.08f, false};
        case RideTrick::PopShoveIt:
            return {RideRotationChannel::BoardDeck, {0.0f, 1.0f, 0.0f},
                    1.0f, kPi, 7.2f, 0.38f, 0.18f, false};
        case RideTrick::Impossible:
            return {RideRotationChannel::BoardDeck, {1.0f, 0.0f, 0.0f},
                    -1.0f, 2.0f * kPi, 10.7f, 0.52f, 0.12f, false};
        case RideTrick::VarialFlip:
            return {RideRotationChannel::BoardDeck, {0.0f, 0.4472136f, 0.8944272f},
                    1.0f, 2.0f * kPi, 11.5f, 0.54f, 0.22f, false};
        case RideTrick::BoardGrab:
            return {RideRotationChannel::BoardDeck, {1.0f, 0.0f, 0.0f},
                    1.0f, 0.72f, 3.8f, 0.38f, 0.10f, true};
        case RideTrick::BmxTailwhipLeft:
            return {RideRotationChannel::BmxFrame, {0.0f, 1.0f, 0.0f},
                    -1.0f, 2.0f * kPi, 10.6f, 0.54f, -0.20f, false};
        case RideTrick::BmxTailwhipRight:
            return {RideRotationChannel::BmxFrame, {0.0f, 1.0f, 0.0f},
                    1.0f, 2.0f * kPi, 10.6f, 0.54f, 0.20f, false};
        case RideTrick::BmxBarspin:
            return {RideRotationChannel::BmxSteering, {0.0f, 1.0f, 0.0f},
                    1.0f, 2.0f * kPi, 13.4f, 0.40f, 0.05f, false};
        case RideTrick::BmxCrankflip:
            return {RideRotationChannel::BmxCrank, {1.0f, 0.0f, 0.0f},
                    -1.0f, 2.0f * kPi, 13.0f, 0.40f, 0.04f, false};
        case RideTrick::BmxXUp:
            return {RideRotationChannel::BmxSteering, {0.0f, 1.0f, 0.0f},
                    1.0f, kPi, 10.2f, 0.46f, 0.12f, true};
        case RideTrick::BmxTabletop:
            return {RideRotationChannel::Rideable, {0.0f, 0.0f, 1.0f},
                    1.0f, 1.05f, 3.8f, 0.48f, 0.18f, true};
        case RideTrick::None:
        case RideTrick::Ollie:
        case RideTrick::BunnyHop:
        case RideTrick::BoardGrind:
        case RideTrick::PegGrind:
        case RideTrick::BoardManual:
        case RideTrick::WheelManual:
        case RideTrick::Land:
        case RideTrick::Bail:
            return {};
    }
    return {};
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

void RideableMovementController::beginPhysicalTrick(
    RideTrick trick,
    RideableFrame& frame
) noexcept
{
    const TrickPhysicalIntent intent = physicalIntentFor(trick);
    beginTrick(trick, frame);
    state_.trickSeconds = 0.0f;
    state_.rideableRotation = {};
    state_.rotationTravel = 0.0f;
    state_.rotationTravelTarget = intent.rotationTarget *
        (intent.returnToNeutral ? 2.0f : 1.0f);
    state_.rotationCompletion = intent.channel == RideRotationChannel::None
        ? 1.0f
        : 0.0f;
    state_.minimumTrickAirtime = intent.minimumAirtime;
    state_.bodyRotationAssist = intent.bodyRotationAssist;
    state_.rotationChannel = intent.channel;
    state_.rotationReturning = false;
    const RideRotation signedAxis = scaled(intent.axis, intent.direction);
    state_.targetRotation = scaled(signedAxis, intent.rotationTarget);
    state_.angularVelocity = scaled(signedAxis, intent.angularSpeed);
}

void RideableMovementController::updatePhysicalRotation(
    float deltaSeconds
) noexcept
{
    if (!state_.flipCommitted || state_.rotationTravelTarget <= 0.0f) {
        return;
    }
    const TrickPhysicalIntent intent = physicalIntentFor(state_.activeTrick);
    if (intent.channel == RideRotationChannel::None) {
        return;
    }
    state_.trickSeconds += deltaSeconds;
    state_.rotationTravel = std::min(
        state_.rotationTravelTarget,
        state_.rotationTravel + intent.angularSpeed * deltaSeconds
    );
    const RideRotation signedAxis = scaled(intent.axis, intent.direction);
    float physicalAngle = state_.rotationTravel;
    float velocityDirection = 1.0f;
    if (intent.returnToNeutral && state_.rotationTravel > intent.rotationTarget) {
        physicalAngle = std::max(
            0.0f,
            intent.rotationTarget * 2.0f - state_.rotationTravel
        );
        velocityDirection = -1.0f;
        state_.rotationReturning = true;
    }
    state_.rideableRotation = scaled(signedAxis, physicalAngle);
    state_.rotationCompletion = std::clamp(
        state_.rotationTravel / state_.rotationTravelTarget,
        0.0f,
        1.0f
    );
    if (state_.rotationCompletion >= 1.0f) {
        state_.angularVelocity = {};
        if (intent.returnToNeutral) {
            state_.rideableRotation = {};
        } else {
            state_.rideableRotation = state_.targetRotation;
        }
    } else {
        state_.angularVelocity = scaled(
            signedAxis,
            intent.angularSpeed * velocityDirection
        );
    }
}

LandingQuality RideableMovementController::evaluateLanding(
    float verticalImpactSpeed,
    float horizontalSpeed,
    const RideRotation& surfaceNormal,
    BailReason& reason
) const noexcept
{
    if (verticalImpactSpeed > tuning_.sketchyImpactSpeed) {
        reason = BailReason::ExcessiveImpact;
        return LandingQuality::Failed;
    }
    if (surfaceNormal.y < 0.70f) {
        reason = BailReason::ContactMisalignment;
        return LandingQuality::Failed;
    }
    if (state_.balance < 12.0f) {
        reason = BailReason::LostBalance;
        return LandingQuality::Bail;
    }
    if (!state_.flipCommitted) {
        if (state_.footContactAlignment < 0.58f) {
            reason = BailReason::ContactMisalignment;
            return LandingQuality::Failed;
        }
        return verticalImpactSpeed <= tuning_.cleanImpactSpeed &&
               horizontalSpeed < 13.0f && state_.balance >= 50.0f
            ? LandingQuality::Clean
            : LandingQuality::Sketchy;
    }

    const TrickPhysicalIntent intent = physicalIntentFor(state_.activeTrick);
    const float orientationError = intent.returnToNeutral
        ? length(state_.rideableRotation)
        : length(difference(state_.rideableRotation, state_.targetRotation));
    const float angularSpeed = length(state_.angularVelocity);
    const bool enoughAir = state_.trickSeconds >= state_.minimumTrickAirtime;
    if (enoughAir && state_.rotationCompletion >= tuning_.cleanCompletion &&
        orientationError <= 0.24f &&
        angularSpeed <= tuning_.cleanAngularSpeed &&
        state_.footContactAlignment >= 0.90f &&
        verticalImpactSpeed <= tuning_.cleanImpactSpeed &&
        horizontalSpeed < 13.5f) {
        return LandingQuality::Clean;
    }
    if (enoughAir && state_.rotationCompletion >= tuning_.sketchyCompletion &&
        orientationError <= 0.78f &&
        angularSpeed <= tuning_.sketchyAngularSpeed &&
        state_.footContactAlignment >= 0.72f &&
        horizontalSpeed < 15.0f) {
        return LandingQuality::Sketchy;
    }
    if (state_.rotationCompletion < tuning_.sketchyCompletion || !enoughAir) {
        reason = BailReason::UnderRotated;
    } else if (angularSpeed > tuning_.sketchyAngularSpeed) {
        reason = BailReason::ExcessiveAngularVelocity;
    } else if (orientationError > kPi * 0.50f) {
        reason = BailReason::InvertedRideable;
    } else {
        reason = BailReason::ContactMisalignment;
    }
    return LandingQuality::Failed;
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
    RideableFrame& frame,
    BailReason reason,
    LandingQuality quality
) noexcept
{
    state_.phase = RidePhase::Crash;
    state_.phaseSeconds = 0.0f;
    state_.activeTrick = RideTrick::Bail;
    state_.landingQuality = quality;
    state_.bailReason = reason;
    state_.activeAffordanceId = 0;
    state_.activeGrindAttachment = RideGrindAttachment::None;
    state_.balance = 0.0f;
    state_.leftHandGripError = state_.discipline == RideDiscipline::BMX ? 0.72f : 0.0f;
    state_.rightHandGripError = state_.discipline == RideDiscipline::BMX ? 0.72f : 0.0f;
    state_.leftFootAnchorError = 0.58f;
    state_.rightFootAnchorError = 0.58f;
    state_.footContactAlignment = 0.0f;
    state_.rideSeparationVelocity = player.velocityX >= 0.0f ? 2.8f : -2.8f;
    if (length(state_.angularVelocity) < 0.5f) {
        state_.angularVelocity = {2.2f, 3.4f, -4.6f};
    }
    appendCombo(RideTrick::Bail);
    player.velocityX *= 0.34f;
    player.velocityZ *= 0.34f;
    frame.bailed = true;
    frame.evaluatedLanding = quality;
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

bool RideableMovementController::validGrindApproach(
    const WorldAffordanceVolume& grind,
    const PlayerState& player
) const noexcept
{
    const RideGrindAttachment opportunity =
        state_.discipline == RideDiscipline::Skateboard
            ? RideGrindAttachment::BoardTrucks
            : state_.discipline == RideDiscipline::BMX
                ? RideGrindAttachment::BmxPegs
                : RideGrindAttachment::None;
    if (opportunity == RideGrindAttachment::None ||
        state_.phase == RidePhase::Crash ||
        state_.speed < kMinimumTrickSpeed) {
        return false;
    }
    const float xSpan = grind.maximumX - grind.minimumX;
    const float zSpan = grind.maximumZ - grind.minimumZ;
    const float inverseSpeed = 1.0f / std::max(state_.speed, 0.001f);
    const float velocityX = player.velocityX * inverseSpeed;
    const float velocityZ = player.velocityZ * inverseSpeed;
    const float alignment = xSpan >= zSpan
        ? std::fabs(velocityX)
        : std::fabs(velocityZ);
    return alignment >= kMinimumGrindAlignment;
}

RideTrick RideableMovementController::trickFor(
    RideDiscipline discipline,
    RideTrickDirection direction
) noexcept
{
    if (discipline == RideDiscipline::Skateboard) {
        switch (direction) {
            case RideTrickDirection::Left: return RideTrick::Kickflip;
            case RideTrickDirection::Right: return RideTrick::Heelflip;
            case RideTrickDirection::Up: return RideTrick::PopShoveIt;
            case RideTrickDirection::Down: return RideTrick::Impossible;
            case RideTrickDirection::UpLeft:
            case RideTrickDirection::UpRight:
            case RideTrickDirection::DownLeft:
            case RideTrickDirection::DownRight: return RideTrick::VarialFlip;
            case RideTrickDirection::None: return RideTrick::None;
        }
    }
    if (discipline == RideDiscipline::BMX) {
        switch (direction) {
            case RideTrickDirection::Left: return RideTrick::BmxTailwhipLeft;
            case RideTrickDirection::Right: return RideTrick::BmxTailwhipRight;
            case RideTrickDirection::Up: return RideTrick::BmxBarspin;
            case RideTrickDirection::Down: return RideTrick::BmxCrankflip;
            case RideTrickDirection::UpLeft:
            case RideTrickDirection::UpRight: return RideTrick::BmxXUp;
            case RideTrickDirection::DownLeft:
            case RideTrickDirection::DownRight: return RideTrick::BmxTabletop;
            case RideTrickDirection::None: return RideTrick::None;
        }
    }
    return RideTrick::None;
}

} // namespace hakui
