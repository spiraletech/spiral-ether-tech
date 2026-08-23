#include "core/HakuiApp.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <string>
#include <utility>

#include "avatar/AvatarGroundContact.hpp"
#include "avatar/RideAttachmentRig.hpp"
#include "observer/NativeFrameCapture.hpp"
#include "render/Math3D.hpp"

#ifndef HAKUI_GIT_SHA
#define HAKUI_GIT_SHA "unknown"
#endif
#ifndef HAKUI_GIT_BRANCH
#define HAKUI_GIT_BRANCH "unknown"
#endif
#ifndef HAKUI_BUILD_TIMESTAMP
#define HAKUI_BUILD_TIMESTAMP "unknown"
#endif
#ifndef HAKUI_BUILD_CONFIGURATION
#define HAKUI_BUILD_CONFIGURATION "unknown"
#endif

namespace {

const char* combatStateLabel(hakui::combat::CombatState state) noexcept
{
    using hakui::combat::CombatState;
    switch (state) {
        case CombatState::Inactive: return "INACTIVE";
        case CombatState::Ready: return "STANCE";
        case CombatState::Guarding: return "GUARD";
        case CombatState::Windup: return "WINDUP";
        case CombatState::Release: return "RELEASE";
        case CombatState::Recovery: return "RECOVERY";
        case CombatState::Staggered: return "HIT REACTION";
        case CombatState::KnockedDown: return "KNOCKDOWN";
    }
    return "UNKNOWN";
}

const char* locomotionLabel(LocomotionMode mode) noexcept
{
    switch (mode) {
        case LocomotionMode::OnFoot: return "ON FOOT";
        case LocomotionMode::Skateboard: return "SKATEBOARD";
        case LocomotionMode::BMX: return "BMX";
        case LocomotionMode::Car: return "CAR DEFERRED";
    }
    return "UNKNOWN";
}

const char* cameraRoleLabel(CameraRole role) noexcept
{
    switch (role) {
        case CameraRole::GameplayFollow: return "gameplay_follow";
        case CameraRole::InteractionFrame: return "interaction_frame";
        case CameraRole::CombatFrame: return "combat_frame";
        case CameraRole::TargetFrame: return "target_frame";
        case CameraRole::DuelFrame: return "duel_frame";
        case CameraRole::Spectator: return "spectator";
        case CameraRole::Director: return "director";
    }
    return "unknown";
}

const char* rideAnchorLabel(hakui::RideAnchorSemantic semantic) noexcept
{
    using hakui::RideAnchorSemantic;
    switch (semantic) {
        case RideAnchorSemantic::None: return "None";
        case RideAnchorSemantic::BikeRoot: return "BikeRoot";
        case RideAnchorSemantic::BoardRoot: return "BoardRoot";
        case RideAnchorSemantic::RiderRoot: return "RiderRoot";
        case RideAnchorSemantic::PelvisAnchor: return "PelvisAnchor";
        case RideAnchorSemantic::LeftFootAnchor: return "LeftFootAnchor";
        case RideAnchorSemantic::RightFootAnchor: return "RightFootAnchor";
        case RideAnchorSemantic::LeftHandGrip: return "LeftHandGrip";
        case RideAnchorSemantic::RightHandGrip: return "RightHandGrip";
        case RideAnchorSemantic::FrontSteeringAssembly: return "FrontSteeringAssembly";
        case RideAnchorSemantic::Fork: return "Fork";
        case RideAnchorSemantic::FrontWheel: return "FrontWheel";
        case RideAnchorSemantic::Stem: return "Stem";
        case RideAnchorSemantic::Handlebar: return "Handlebar";
        case RideAnchorSemantic::FrontAxle: return "FrontAxle";
        case RideAnchorSemantic::RearAxle: return "RearAxle";
        case RideAnchorSemantic::Crank: return "Crank";
        case RideAnchorSemantic::LeftPedal: return "LeftPedal";
        case RideAnchorSemantic::RightPedal: return "RightPedal";
        case RideAnchorSemantic::SeatAnchor: return "SeatAnchor";
        case RideAnchorSemantic::BoardDeckAnchor: return "BoardDeckAnchor";
        case RideAnchorSemantic::FrontFootAnchor: return "FrontFootAnchor";
        case RideAnchorSemantic::RearFootAnchor: return "RearFootAnchor";
        case RideAnchorSemantic::FrontTruck: return "FrontTruck";
        case RideAnchorSemantic::RearTruck: return "RearTruck";
        case RideAnchorSemantic::DeckCenter: return "DeckCenter";
        case RideAnchorSemantic::Count: break;
    }
    return "UnknownAnchor";
}

hakui::RideTrickDirection rideTrickDirection(
    hakui::input::FlickDirection direction
) noexcept
{
    using InputDirection = hakui::input::FlickDirection;
    using RideDirection = hakui::RideTrickDirection;
    switch (direction) {
        case InputDirection::None: return RideDirection::None;
        case InputDirection::Left: return RideDirection::Left;
        case InputDirection::Right: return RideDirection::Right;
        case InputDirection::Up: return RideDirection::Up;
        case InputDirection::Down: return RideDirection::Down;
        case InputDirection::UpLeft: return RideDirection::UpLeft;
        case InputDirection::UpRight: return RideDirection::UpRight;
        case InputDirection::DownLeft: return RideDirection::DownLeft;
        case InputDirection::DownRight: return RideDirection::DownRight;
    }
    return RideDirection::None;
}

hakui::EmbodimentProfileId embodimentProfile(
    const PlayerState& player,
    const hakui::combat::CombatSimulation& combat
) noexcept
{
    if (combat.active()) {
        return combat.player().state == hakui::combat::CombatState::KnockedDown
            ? hakui::EmbodimentProfileId::Knockdown
            : hakui::EmbodimentProfileId::Combat;
    }
    if (player.activity != PlayerActivity::Roaming) {
        return hakui::EmbodimentProfileId::Seated;
    }
    if (player.locomotion == LocomotionMode::Skateboard) {
        return hakui::EmbodimentProfileId::Skateboard;
    }
    if (player.locomotion == LocomotionMode::BMX) {
        return hakui::EmbodimentProfileId::Bmx;
    }
    return hakui::EmbodimentProfileId::OnFoot;
}

} // namespace

bool HakuiApp::boot()
{
    SDL_Log("[HAKUI] booting native client v0.84-dev // RIDE PHYSICS + EMBODIMENT");

    if (!initPlatform() || !initGPU()) {
        return false;
    }

    if (!avatarSkeleton_.buildDefaultHumanoid()) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "[HAKUI] avatar rig initialization failed"
        );
        return false;
    }

    initSpiralCore();
    terminal_ = std::make_shared<hakui::games::GameTerminal>(
        7001,
        hakui::games::TerminalModel::FusionDeck
    );
    if (!interactions_.registerTarget(terminal_)) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "[HAKUI] failed to register tabletop terminal"
        );
        return false;
    }
    const hakui::MovementEnvironment room = blackRoom_.movementEnvironment();
    player_.x = room.spawnX;
    player_.y = room.spawnY;
    player_.z = room.spawnZ;
    player_.money = 250.0f;

    (void)audio_.init();
    int gamepadCount = 0;
    SDL_JoystickID* gamepads = SDL_GetGamepads(&gamepadCount);
    if (gamepads && gamepadCount > 0) {
        openGamepad(gamepads[0]);
    }
    SDL_free(gamepads);
    previousCounter_ = SDL_GetPerformanceCounter();

    SDL_Log("[HAKUI] WORLD ONLINE");
    SDL_Log("[HAKUI] DATA GRUNGE // ACTIVE");
    SDL_Log("[HAKUI] SPIRAL CORE // ONLINE");
    SDL_Log("[HAKUI] avatar skeleton // %zu bones loaded", avatarSkeleton_.boneCount());
    SDL_Log("[HAKUI] v0.84 ride physics // preload -> pop -> flick -> land or bail online");
    SDL_Log("[HAKUI] procedural locomotion // idle + walk + sprint + jump + seated online");
    SDL_Log("[HAKUI] BLACK ROOM // neon lounge + couch + fusion table + open void");
    SDL_Log("[HAKUI] controls // WASD move // SPACE jump // E interact/stand // SHIFT sprint");
    SDL_Log("[HAKUI] camera // RMB orbit // WHEEL zoom // R reset");
    SDL_Log("[HAKUI] menu // ESC pause // [ ] look sensitivity // - + audio // F10 quit");
    SDL_Log("[HAKUI] ride interface // controller native // keyboard fallback F9 developer-only");
    SDL_Log("[HAKUI] ride verbs // SOUTH pop // THEN RS flick in air // NORTH grind // LT balance // RT drive");
    SDL_Log("[HAKUI] ride expression // LB/RB spin // WEST style // EAST dismount");
    SDL_Log("[HAKUI] modes // DPAD UP foot // LEFT skateboard // RIGHT BMX");
    SDL_Log("[HAKUI] FUSION TABLE // E contextual action // C leave // virtual credits only");
    SDL_Log("[HAKUI] expert observer // F12 read-only inspection bundle");
    SDL_Log("[HAKUI] combat grammar // semantic primary/secondary // guard // recover");
    SDL_Log("[HAKUI] combat foundation // unarmed playable // sword + bow extension seams dormant");
    recordObserverEvent("boot", "WORLD ONLINE // observer read-only boundary ready");
    return true;
}

bool HakuiApp::initPlatform()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "[HAKUI] SDL init failed: %s",
            SDL_GetError()
        );
        return false;
    }

    window_ = SDL_CreateWindow(
        "SPIRAL OS: HAKUI ENGINE // v0.84-dev // RIDE PHYSICS + EMBODIMENT",
        1280,
        720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );

    if (!window_) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "[HAKUI] window creation failed: %s",
            SDL_GetError()
        );
        return false;
    }

    return true;
}

bool HakuiApp::initGPU()
{
    constexpr SDL_GPUShaderFormat formats = static_cast<SDL_GPUShaderFormat>(
        SDL_GPU_SHADERFORMAT_SPIRV |
        SDL_GPU_SHADERFORMAT_DXIL |
        SDL_GPU_SHADERFORMAT_MSL
    );

    gpu_ = SDL_CreateGPUDevice(formats, true, nullptr);
    if (!gpu_) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "[HAKUI] GPU device creation failed: %s",
            SDL_GetError()
        );
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(gpu_, window_)) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "[HAKUI] GPU could not claim window: %s",
            SDL_GetError()
        );
        return false;
    }

    SDL_Log("[HAKUI] renderer backend: %s", SDL_GetGPUDeviceDriver(gpu_));

    if (!debugRenderer_.init(gpu_, window_)) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "[HAKUI] renderer initialization failed: %s",
            SDL_GetError()
        );
        return false;
    }

    return true;
}

void HakuiApp::initSpiralCore()
{
    // Bridge only telemetry upward into SDL. Spiral Core itself remains free of
    // SDL/platform dependencies.
    spiralListener_ = spiral_.router().subscribe([](const spiral::Signal& signal) {
        if (signal.kind == spiral::SignalKind::Error) {
            SDL_LogError(
                SDL_LOG_CATEGORY_APPLICATION,
                "[SPIRAL] error // %s // %s",
                signal.topic.c_str(),
                signal.payload.c_str()
            );
        }
    });

    spiral::Signal bootSignal;
    bootSignal.kind = spiral::SignalKind::Boot;
    bootSignal.source = "hakui.client";
    bootSignal.destination = "spiral.core";
    bootSignal.topic = "client.boot";
    bootSignal.payload = "hakui-v0.84-dev";

    // A small initial charge marks the native client's transition to live
    // operation. Steam is telemetry/energy state, not gameplay policy.
    spiral_.dispatch(std::move(bootSignal), 4.0f, 0.5f);

    // Canonical state lives in StateStore through typed State patches. Boot is
    // kept as its own event family rather than overloading it with reducer data.
    spiral::Signal initialState;
    initialState.kind = spiral::SignalKind::State;
    initialState.source = "hakui.client";
    initialState.destination = "spiral.core";
    initialState.topic = "client.state.initial";
    initialState.statePatch = {
        {"client.status", std::string("online")},
        {"client.version", std::string("0.84-dev")},
        {"avatar.rig.bones", static_cast<std::int64_t>(avatarSkeleton_.boneCount())},
        {"player.locomotion", std::string("on_foot")}
    };
    spiral_.dispatch(std::move(initialState));
}

void HakuiApp::switchLocomotion(LocomotionMode mode, std::string_view label)
{
    if (player_.activity != PlayerActivity::Roaming || combat_.active()) {
        SDL_Log("[HAKUI] locomotion change blocked by active interaction");
        return;
    }

    player_.velocityX = 0.0f;
    player_.velocityY = 0.0f;
    player_.velocityZ = 0.0f;
    player_.movementBlend = 0.0f;
    rideable_.reset();
    rideControls_.reset();
    rideControlFrame_ = {};
    locomotion_.switchTo(mode);

    if (mode == LocomotionMode::Car) {
        SDL_Log("[HAKUI] CAR // representation and controller remain deferred");
    }

    // Gameplay input remains immediate. We publish the resulting state change
    // after the fact; Ether Bus is reserved for transitions that actually need
    // its ritualized transit gate.
    spiral::Signal signal;
    signal.kind = spiral::SignalKind::State;
    signal.source = "hakui.input";
    signal.destination = "spiral.core";
    signal.topic = "locomotion.changed";
    signal.payload = std::string(label);
    signal.statePatch = {
        {"player.locomotion", std::string(label)}
    };

    spiral_.dispatch(std::move(signal), 0.25f, 0.01f);
    recordObserverEvent("locomotion", label);
}

void HakuiApp::requestRideLocomotion(
    LocomotionMode mode,
    std::string_view label
)
{
    const bool advancedRide = mode == LocomotionMode::Skateboard ||
        mode == LocomotionMode::BMX;
    if (advancedRide && !inputFrame_.gamepadAvailable &&
        !developerRideFallback_) {
        showInputStatus(
            "RIDE INPUT UNAVAILABLE // CONNECT GAMEPAD TO SYNCHRONIZE",
            4.0f
        );
        SDL_Log("[RIDE] input unavailable // connect SDL gamepad");
        return;
    }

    switchLocomotion(mode, label);
    if (advancedRide && developerRideFallback_ &&
        !inputFrame_.gamepadAvailable) {
        showInputStatus("DEVELOPER RIDE FALLBACK // KEYBOARD INPUT ACTIVE");
    }
}

void HakuiApp::showInputStatus(std::string message, float seconds)
{
    inputStatus_ = std::move(message);
    inputStatusTimer_ = std::max(0.0f, seconds);
}

void HakuiApp::recordObserverEvent(
    std::string_view category,
    std::string_view message
)
{
    observerJournal_.record(world_.elapsedSeconds, category, message);
}

hakui::observer::CaptureContext HakuiApp::buildObserverContext() const
{
    namespace observer = hakui::observer;

    observer::CaptureContext context;
    context.build.hakuiVersion = "0.84-dev";
    context.build.configuration = HAKUI_BUILD_CONFIGURATION;
    context.build.gitCommit = HAKUI_GIT_SHA;
    context.build.gitBranch = HAKUI_GIT_BRANCH;
    context.build.buildTimestamp = HAKUI_BUILD_TIMESTAMP;
    context.build.platform = SDL_GetPlatform();
    context.build.rendererBackend = gpu_ ? SDL_GetGPUDeviceDriver(gpu_) : "offline";
    context.geometry = blackRoom_.geometry();
    context.affordances = blackRoom_.affordances();

    const hakui::MovementEnvironment environment =
        blackRoom_.movementEnvironment();
    context.spawn = {
        environment.spawnX,
        environment.spawnY,
        environment.spawnZ
    };
    context.voidResetHeight = environment.voidResetHeight;
    context.input = inputFrame_;
    context.connectedGamepads = gamepad_ ? 1U : 0U;
    context.rideEligible = inputFrame_.gamepadAvailable || developerRideFallback_;
    context.rideControl.activeDiscipline = combat_.active()
        ? "BOXING"
        : player_.activity != PlayerActivity::Roaming
            ? "SEATED"
            : player_.locomotion == LocomotionMode::Skateboard
                ? "SKATEBOARD"
                : player_.locomotion == LocomotionMode::BMX
                    ? "BMX"
                    : "ON_FOOT";
    context.rideControl.popIntent = rideControlFrame_.popIntent;
    context.rideControl.popPreparing = rideControlFrame_.popPreparing;
    context.rideControl.popPreload = rideControlFrame_.popPreload;
    context.rideControl.airborne = rideControlFrame_.airborne;
    context.rideControl.trickWindowArmed = rideControlFrame_.trickWindowArmed;
    context.rideControl.trickWindowRemaining =
        rideControlFrame_.trickWindowRemaining;
    context.rideControl.rawRightStickX = rideControlFrame_.rawRightStickX;
    context.rideControl.rawRightStickY = rideControlFrame_.rawRightStickY;
    context.rideControl.normalizedFlickX = rideControlFrame_.normalizedFlickX;
    context.rideControl.normalizedFlickY = rideControlFrame_.normalizedFlickY;
    context.rideControl.detectedFlick = std::string(
        hakui::input::RideControlInterpreter::directionName(
            rideControlFrame_.trick.direction
        )
    );
    context.rideControl.trickIntent = rideControlFrame_.trickIntent;
    context.rideControl.cameraOwnsRightStick =
        rideControlFrame_.rightStickOwner ==
        hakui::input::RightStickOwner::Camera;
    context.rideControl.grindIntent = rideControlFrame_.grindIntent;
    context.rideControl.balanceIntent = rideControlFrame_.balanceIntent;
    context.rideControl.propulsion = rideControlFrame_.propulsionIntent;
    context.rideControl.spinLeft = rideControlFrame_.spinLeftIntent;
    context.rideControl.spinRight = rideControlFrame_.spinRightIntent;
    context.rideControl.rightStickOwner = std::string(
        hakui::input::RideControlInterpreter::ownerName(
            rideControlFrame_.rightStickOwner
        )
    );
    context.rideControl.rideState =
        player_.locomotion == LocomotionMode::Skateboard ||
        player_.locomotion == LocomotionMode::BMX
            ? std::string(hakui::RideableMovementController::phaseLabel(
                rideable_.state().phase
            ))
            : "INACTIVE";
    const hakui::input::RideControlTuning& rideTuning = rideControls_.tuning();
    context.rideControl.cameraDeadzone = rideTuning.cameraDeadzone;
    context.rideControl.preloadSaturationTime = rideTuning.preloadSaturationTime;
    context.rideControl.trickWindowDelay = rideTuning.trickWindowDelay;
    context.rideControl.trickWindowDuration = rideTuning.trickWindowDuration;
    context.rideControl.flickDeadzone = rideTuning.flickDeadzone;
    context.rideControl.flickThreshold = rideTuning.flickThreshold;
    context.rideControl.flickReleaseThreshold = rideTuning.flickReleaseThreshold;

    if (player_.activity == PlayerActivity::CasinoSeated && terminal_) {
        context.currentInteractionIntent = std::string(
            hakui::games::GameTerminal::contextActionLabel(
                terminal_->nextContextAction()
            )
        );
    } else if (player_.activity == PlayerActivity::CouchSeated) {
        context.currentInteractionIntent = "STAND";
    } else if (blackRoom_.nearestInteraction(player_)) {
        context.currentInteractionIntent = "INTERACT";
    }

    std::string worldZone = "unbounded_void";
    for (const hakui::WorldAffordanceVolume& volume : context.affordances) {
        if (volume.contains(player_.x, player_.y, player_.z)) {
            worldZone = std::string(volume.label);
            break;
        }
    }

    const hakui::EmbodimentProfileId profileId =
        embodimentProfile(player_, combat_);
    const std::string movementState = combat_.active()
        ? "combat"
        : player_.activity != PlayerActivity::Roaming
            ? "seated"
            : !player_.grounded
                ? "airborne"
                : player_.movementBlend > 0.80f
                    ? "sprint"
                    : player_.movementBlend > 0.05f ? "walk" : "idle";

    observer::EntityObservation playerEntity;
    playerEntity.id = 1;
    playerEntity.type = "PlayerAvatar";
    playerEntity.position = {player_.x, player_.y, player_.z};
    playerEntity.velocity = {
        player_.velocityX,
        player_.velocityY,
        player_.velocityZ
    };
    playerEntity.yaw = player_.yaw;
    playerEntity.grounded = player_.grounded;
    playerEntity.locomotion = locomotionLabel(player_.locomotion);
    playerEntity.movementState = movementState;
    playerEntity.animationState = std::string(
        hakui::embodimentProfileName(profileId)
    ) + ":" + movementState;
    playerEntity.combatState = combat_.active()
        ? combatStateLabel(combat_.player().state)
        : "INACTIVE";
    playerEntity.health = combat_.active() ? combat_.player().health : player_.health;
    playerEntity.stamina = combat_.active() ? combat_.player().stamina : player_.stamina;
    playerEntity.balance = (player_.locomotion == LocomotionMode::Skateboard ||
                            player_.locomotion == LocomotionMode::BMX)
        ? rideable_.state().balance
        : combat_.active() ? combat_.player().balance : 100.0f;
    playerEntity.currentRideable =
        player_.locomotion == LocomotionMode::Skateboard ? "rideable.skateboard" :
        player_.locomotion == LocomotionMode::BMX ? "rideable.bmx" : "none";
    playerEntity.currentInteraction =
        player_.activity == PlayerActivity::CasinoSeated ? "casino_seated" :
        player_.activity == PlayerActivity::CouchSeated ? "couch_seated" :
        combat_.active() ? "sparring" : "none";
    playerEntity.currentWorldZone = worldZone;
    playerEntity.attachments = {
        {"PelvisAnchor", "player.1", {0.0f, 1.0f, 0.0f}},
        {"LeftFootAnchor", "player.1", {-0.18f, 0.0f, 0.0f}},
        {"RightFootAnchor", "player.1", {0.18f, 0.0f, 0.0f}}
    };
    context.entities.push_back(std::move(playerEntity));

    if (player_.locomotion == LocomotionMode::Skateboard ||
        player_.locomotion == LocomotionMode::BMX) {
        const bool skateboard = player_.locomotion == LocomotionMode::Skateboard;
        const hakui::RideAttachmentRig rig = skateboard
            ? hakui::RideAttachmentRig::skateboard()
            : hakui::RideAttachmentRig::bmx();
        observer::EntityObservation rideableEntity;
        rideableEntity.id = skateboard ? 2001U : 2002U;
        rideableEntity.type = skateboard ? "Skateboard" : "BMX";
        rideableEntity.parent = "player.1";
        rideableEntity.position = {player_.x, player_.y, player_.z};
        rideableEntity.velocity = {
            player_.velocityX,
            player_.velocityY,
            player_.velocityZ
        };
        rideableEntity.yaw = player_.yaw;
        rideableEntity.grounded = player_.grounded;
        rideableEntity.locomotion = locomotionLabel(player_.locomotion);
        rideableEntity.movementState = std::string(
            hakui::RideableMovementController::phaseLabel(
                rideable_.state().phase
            )
        );
        rideableEntity.animationState = std::string(
            hakui::RideableMovementController::trickLabel(
                rideable_.state().activeTrick
            )
        );
        rideableEntity.balance = rideable_.state().balance;
        const hakui::RideableState& rideState = rideable_.state();
        rideableEntity.rideDiscipline = skateboard ? "SKATEBOARD" : "BMX";
        rideableEntity.rideState = std::string(
            hakui::RideableMovementController::phaseLabel(rideState.phase)
        );
        rideableEntity.currentTrick = std::string(
            hakui::RideableMovementController::trickLabel(rideState.activeTrick)
        );
        rideableEntity.rotationChannel = std::string(
            hakui::RideableMovementController::rotationChannelLabel(
                rideState.rotationChannel
            )
        );
        rideableEntity.landingQuality = std::string(
            hakui::RideableMovementController::landingLabel(
                rideState.landingQuality
            )
        );
        rideableEntity.bailReason = std::string(
            hakui::RideableMovementController::bailReasonLabel(
                rideState.bailReason
            )
        );
        rideableEntity.popPreload = rideState.popPreload;
        rideableEntity.popImpulse = rideState.popImpulse;
        rideableEntity.airtime = rideState.airSeconds;
        rideableEntity.rideableRotation = {
            rideState.rideableRotation.x,
            rideState.rideableRotation.y,
            rideState.rideableRotation.z
        };
        rideableEntity.angularVelocity = {
            rideState.angularVelocity.x,
            rideState.angularVelocity.y,
            rideState.angularVelocity.z
        };
        rideableEntity.targetRotation = {
            rideState.targetRotation.x,
            rideState.targetRotation.y,
            rideState.targetRotation.z
        };
        rideableEntity.rotationCompletion = rideState.rotationCompletion;
        rideableEntity.leftHandGripError = rideState.leftHandGripError;
        rideableEntity.rightHandGripError = rideState.rightHandGripError;
        rideableEntity.leftFootAnchorError = rideState.leftFootAnchorError;
        rideableEntity.rightFootAnchorError = rideState.rightFootAnchorError;
        for (const hakui::RideAnchor& anchor : rig.anchors()) {
            rideableEntity.attachments.push_back({
                rideAnchorLabel(anchor.semantic),
                anchor.parent == hakui::RideAnchorSemantic::None
                    ? (skateboard ? "rideable.skateboard" : "rideable.bmx")
                    : rideAnchorLabel(anchor.parent),
                {anchor.x, anchor.y, anchor.z}
            });
        }
        context.entities.push_back(std::move(rideableEntity));
    }

    if (const hakui::WorldAffordanceVolume* sparZone =
            blackRoom_.firstAffordance(hakui::WorldAffordance::FightZone)) {
        observer::EntityObservation opponent;
        opponent.id = combat_.opponent().entity;
        opponent.type = "SparringDummy";
        opponent.position = combat_.active()
            ? observer::Vec3{
                combat_.opponentWorldPosition().x,
                combat_.opponentWorldPosition().y,
                combat_.opponentWorldPosition().z
            }
            : observer::Vec3{
                sparZone->secondaryAnchor.x,
                sparZone->secondaryAnchor.y,
                sparZone->secondaryAnchor.z
            };
        opponent.yaw = sparZone->secondaryAnchor.yaw;
        opponent.grounded = true;
        opponent.locomotion = "ON FOOT";
        opponent.movementState = combat_.active() ? "combat" : "idle";
        opponent.animationState = opponent.movementState;
        opponent.combatState = combatStateLabel(combat_.opponent().state);
        opponent.health = combat_.opponent().health;
        opponent.stamina = combat_.opponent().stamina;
        opponent.balance = combat_.opponent().balance;
        opponent.currentWorldZone = std::string(sparZone->label);
        context.entities.push_back(std::move(opponent));
    }

    context.camera.mode = cameraRoleLabel(debugRenderer_.cameraRole());
    context.camera.position = {
        debugRenderer_.cameraWorldX(),
        debugRenderer_.cameraWorldY(),
        debugRenderer_.cameraWorldZ()
    };
    context.camera.target = {
        debugRenderer_.cameraTargetX(),
        debugRenderer_.cameraTargetY(),
        debugRenderer_.cameraTargetZ()
    };
    context.camera.targetEntity = combat_.active() ? "encounter.sparring" : "player.1";
    context.camera.yaw = debugRenderer_.cameraYaw();
    context.camera.pitch = debugRenderer_.cameraPitch();
    context.camera.distance = debugRenderer_.cameraDistance();
    context.camera.fieldOfViewDegrees = debugRenderer_.fieldOfViewDegrees();
    context.camera.orbiting = cameraDragging_;
    context.camera.inputOwner = std::string(
        hakui::input::InputResolver::deviceName(inputFrame_.activeDevice)
    );
    if (inputFrame_.activeDevice == hakui::input::InputDevice::Gamepad) {
        context.camera.inputOwner += ":";
        context.camera.inputOwner +=
            hakui::input::RideControlInterpreter::ownerName(
                rideControlFrame_.rightStickOwner
            );
    }

    context.runtime.elapsedSeconds = world_.elapsedSeconds;
    context.runtime.paused = paused_;
    context.runtime.worldRevision = spiral_.stateStore().revision();
    context.runtime.recentEvents = observerJournal_.entries();
    return context;
}

void HakuiApp::captureExpertSnapshot()
{
    std::error_code pathError;
    const std::filesystem::path workingDirectory =
        std::filesystem::current_path(pathError);
    if (pathError) {
        showInputStatus("EXPERT SNAPSHOT FAILED // OUTPUT PATH UNAVAILABLE", 4.0f);
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "[OBSERVER] output path unavailable: %s",
            pathError.message().c_str()
        );
        return;
    }

    hakui::observer::CaptureContext context = buildObserverContext();
    const hakui::observer::ExportResult result =
        hakui::observer::ExpertObserver::capture(
            context,
            workingDirectory / "HAKUI-OBSERVE",
            [this](const std::filesystem::path& destination, std::string& error) {
                return hakui::observer::captureWindowPng(
                    window_, destination, error
                );
            }
        );
    if (!result.success) {
        recordObserverEvent("observer.error", result.error);
        showInputStatus("EXPERT SNAPSHOT FAILED // SEE RUNTIME LOG", 4.0f);
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "[OBSERVER] capture failed // %s",
            result.error.c_str()
        );
        return;
    }

    lastObserverBundle_ = result.bundlePath;
    recordObserverEvent("observer.capture", result.captureId);
    showInputStatus("EXPERT SNAPSHOT WRITTEN // HAKUI-OBSERVE", 4.0f);
    SDL_Log(
        "[OBSERVER] read-only bundle // %s",
        result.bundlePath.string().c_str()
    );
}

void HakuiApp::interactWithTerminal(hakui::InteractionVerb verb)
{
    if (!terminal_ || player_.activity != PlayerActivity::CasinoSeated) {
        SDL_Log("[TABLETOP] controls unavailable // sit at the Fusion table first");
        return;
    }

    hakui::InteractionRequest request;
    request.actor = 1;
    request.target = terminal_->interactionId();
    request.verb = verb;

    const hakui::InteractionResult result = interactions_.interact(request);
    if (result.handled) {
        SDL_Log("[TERMINAL] %s", result.output.c_str());
        audio_.play(
            verb == hakui::InteractionVerb::Use
                ? HakuiAudioCue::Casino
                : HakuiAudioCue::Interact
        );
    } else {
        SDL_Log("[TERMINAL] action unavailable");
    }
}

void HakuiApp::handleCasinoContextAction()
{
    if (player_.activity != PlayerActivity::CasinoSeated || !terminal_) {
        SDL_Log("[TABLETOP] control anchored to table // press E at the chair");
        return;
    }

    using hakui::games::TerminalContextAction;
    const TerminalContextAction action = terminal_->nextContextAction();
    switch (action) {
        case TerminalContextAction::PowerOn:
            interactWithTerminal(hakui::InteractionVerb::Use);
            break;

        case TerminalContextAction::OpenCards:
            interactWithTerminal(hakui::InteractionVerb::Play);
            break;

        case TerminalContextAction::Bet25:
            if (terminal_->beginCardRound(25)) {
                SDL_Log("[TABLETOP] round started // virtual wager 25");
                audio_.play(HakuiAudioCue::Casino);
            }
            break;

        case TerminalContextAction::Hit:
            if (terminal_->hitCardTable()) {
                SDL_Log(
                    "[TABLETOP] hit // hand value %d",
                    hakui::games::BlackjackTable::handValue(
                        terminal_->cardTable().playerHand()
                    )
                );
                audio_.play(HakuiAudioCue::Interact);
            }
            break;

        case TerminalContextAction::Stand:
            if (terminal_->standCardTable()) {
                SDL_Log(
                    "[TABLETOP] stand // virtual credits %lld",
                    static_cast<long long>(terminal_->virtualCredits())
                );
                audio_.play(HakuiAudioCue::Casino);
            }
            break;

    }
}

void HakuiApp::leaveCurrentInteraction()
{
    if (blackRoom_.leaveInteraction(player_)) {
        debugRenderer_.setCameraRole(CameraRole::GameplayFollow);
        audio_.play(HakuiAudioCue::Interact);
        SDL_Log("[HAKUI] interaction released // movement restored");
        recordObserverEvent("interaction", "released");
    }
}

void HakuiApp::handlePrimaryInteraction()
{
    if (combat_.active()) {
        SDL_Log("[COMBAT] interaction locked during spar // C to leave");
        return;
    }

    if (player_.activity != PlayerActivity::Roaming) {
        if (player_.activity == PlayerActivity::CasinoSeated) {
            handleCasinoContextAction();
        } else {
            leaveCurrentInteraction();
        }
        return;
    }

    const hakui::RoomInteractionFocus focus =
        blackRoom_.nearestInteraction(player_);
    if (!focus) {
        SDL_Log("[HAKUI] no interaction in range");
        return;
    }

    if (player_.locomotion != LocomotionMode::OnFoot) {
        switchLocomotion(LocomotionMode::OnFoot, "on_foot");
    }
    if (!blackRoom_.engageNearest(player_)) {
        SDL_Log("[HAKUI] interaction anchor became unavailable");
        return;
    }

    if (player_.activity == PlayerActivity::CasinoSeated) {
        debugRenderer_.frameInteraction(InteractionFrame::FusionTable);
        if (terminal_ && !terminal_->powered()) {
            interactWithTerminal(hakui::InteractionVerb::Use);
        }
        SDL_Log("[TABLETOP] seated // controls unlocked // virtual credits only");
        recordObserverEvent("interaction", "fusion_table seated");
        audio_.play(HakuiAudioCue::Casino);
    } else {
        debugRenderer_.frameInteraction(InteractionFrame::LoungeCouch);
        SDL_Log("[HAKUI] seated // VOID COUCH // E to stand");
        recordObserverEvent("interaction", "void_couch seated");
        audio_.play(HakuiAudioCue::Interact);
    }
}

void HakuiApp::toggleCombat()
{
    if (combat_.active()) {
        const hakui::combat::CombatVector exitAnchor =
            combat_.zone().playerAnchor;
        combat_.leave();
        player_.x = exitAnchor.x - 0.90f;
        player_.y = exitAnchor.y;
        player_.z = exitAnchor.z;
        player_.yaw = 1.57079632679f;
        debugRenderer_.setCameraRole(CameraRole::GameplayFollow);
        audio_.play(HakuiAudioCue::Interact);
        SDL_Log("[COMBAT] spar released // free movement restored");
        recordObserverEvent("combat", "spar released");
        return;
    }

    const hakui::WorldAffordanceVolume* fightZone =
        blackRoom_.affordanceAt(
            hakui::WorldAffordance::FightZone,
            player_.x,
            player_.y,
            player_.z
        );
    if (player_.activity != PlayerActivity::Roaming || !player_.grounded ||
        !fightZone) {
        SDL_Log("[COMBAT] find the red/cyan SPARRING DATUM // C to enter");
        return;
    }

    const hakui::combat::CombatZone zone{
        fightZone->id,
        {
            fightZone->primaryAnchor.x,
            fightZone->primaryAnchor.y,
            fightZone->primaryAnchor.z
        },
        {
            fightZone->secondaryAnchor.x,
            fightZone->secondaryAnchor.y,
            fightZone->secondaryAnchor.z
        }
    };
    if (player_.locomotion != LocomotionMode::OnFoot) {
        switchLocomotion(LocomotionMode::OnFoot, "on_foot");
    }
    if (!combat_.enter(zone)) {
        return;
    }

    player_.x = fightZone->primaryAnchor.x;
    player_.y = fightZone->primaryAnchor.y;
    player_.z = fightZone->primaryAnchor.z;
    player_.yaw = fightZone->primaryAnchor.yaw;
    player_.velocityX = 0.0f;
    player_.velocityY = 0.0f;
    player_.velocityZ = 0.0f;
    player_.movementBlend = 0.0f;
    opponentDecisionTimer_ = 1.15f;
    opponentCrossNext_ = false;
    debugRenderer_.setCombatTarget(
        fightZone->secondaryAnchor.x,
        fightZone->secondaryAnchor.y + 1.25f,
        fightZone->secondaryAnchor.z
    );
    debugRenderer_.setCameraRole(CameraRole::CombatFrame);
    audio_.play(HakuiAudioCue::Interact);
    SDL_Log("[COMBAT] SPARRING DATUM entered // third-person combat frame online");
    recordObserverEvent("combat", "spar entered");
}

void HakuiApp::updateCombat(
    float dt,
    const hakui::input::DisciplineIntent& intent
)
{
    if (!combat_.active()) {
        return;
    }

    using namespace hakui::combat;
    CombatIntent playerIntent;
    if (intent.primary) {
        playerIntent.attack = AttackSemantic::Jab;
    } else if (intent.secondary) {
        playerIntent.attack = AttackSemantic::Cross;
    }
    playerIntent.recover = intent.recover;
    playerIntent.defense = intent.guard
        ? DefenseIntent::Guard
        : DefenseIntent::None;
    playerIntent.moveForward = intent.moveForward;
    playerIntent.moveRight = intent.moveRight;

    CombatIntent opponentIntent;
    if (combat_.opponent().state == CombatState::KnockedDown) {
        opponentIntent.recover = combat_.opponent().phaseSeconds <= 0.0f;
    } else {
        opponentDecisionTimer_ -= dt;
        if (opponentDecisionTimer_ <= 0.0f &&
            combat_.opponent().state == CombatState::Ready) {
            opponentIntent.attack = opponentCrossNext_
                ? AttackSemantic::Cross
                : AttackSemantic::Jab;
            opponentCrossNext_ = !opponentCrossNext_;
            opponentDecisionTimer_ = opponentCrossNext_ ? 1.20f : 1.45f;
        }
    }

    const CombatFrameResult frame = combat_.update(
        playerIntent,
        opponentIntent,
        {
            combat_.zone().playerAnchor,
            combat_.zone().opponentAnchor
        },
        dt
    );
    const CombatVector& playerPosition = combat_.playerWorldPosition();
    const CombatVector& opponentPosition = combat_.opponentWorldPosition();
    player_.x = playerPosition.x;
    player_.y = playerPosition.y;
    player_.z = playerPosition.z;
    player_.yaw = std::atan2(
        opponentPosition.x - playerPosition.x,
        opponentPosition.z - playerPosition.z
    );
    player_.movementBlend = combat_.player().stanceBlend;
    if (combat_.player().stanceBlend > 0.01f) {
        player_.gaitPhase += 9.5f * dt;
    }
    if (frame.playerAttackStarted || frame.opponentAttackStarted) {
        audio_.play(HakuiAudioCue::CombatSwing);
    }
    for (std::size_t index = 0; index < frame.damageEventCount; ++index) {
        const DamageEvent& damage = frame.damageEvents[index];
        if (damage.result == HitResult::Miss) {
            continue;
        }
        if (damage.target == combat_.player().entity) {
            playerHitPulse_ = 0.26f;
        } else {
            opponentHitPulse_ = 0.26f;
        }
        if (damage.result == HitResult::Guarded) {
            audio_.play(HakuiAudioCue::CombatGuard);
        } else if (damage.result == HitResult::Knockdown) {
            audio_.play(HakuiAudioCue::Knockdown);
        } else {
            audio_.play(HakuiAudioCue::CombatHit);
        }
        SDL_Log(
            "[COMBAT] %u -> %u // result %d // damage %.1f // balance %.1f",
            damage.source,
            damage.target,
            static_cast<int>(damage.result),
            damage.damage,
            damage.target == combat_.player().entity
                ? combat_.player().balance
                : combat_.opponent().balance
        );
    }
    if (frame.playerRecovered || frame.opponentRecovered) {
        audio_.play(HakuiAudioCue::Recovery);
    }

    playerHitPulse_ = std::max(0.0f, playerHitPulse_ - dt);
    opponentHitPulse_ = std::max(0.0f, opponentHitPulse_ - dt);
}

void HakuiApp::setPaused(bool paused)
{
    paused_ = paused;
    rideControls_.reset();
    rideControlFrame_ = {};
    if (paused_) {
        (void)setCameraCapture(false);
    }
    SDL_Log(paused_ ? "[HAKUI] PAUSED" : "[HAKUI] RESUMED");
    recordObserverEvent("runtime", paused_ ? "paused" : "resumed");
}

bool HakuiApp::setCameraCapture(bool enabled)
{
    if (!window_) {
        cameraDragging_ = false;
        return false;
    }

    if (!enabled) {
        const bool wasCaptured = SDL_GetWindowRelativeMouseMode(window_);
        const bool released = !wasCaptured ||
            SDL_SetWindowRelativeMouseMode(window_, false);
        cameraDragging_ = false;
        cameraCaptureWarmupFrames_ = 0;
        float discardedX = 0.0f;
        float discardedY = 0.0f;
        (void)SDL_GetRelativeMouseState(&discardedX, &discardedY);
        if (!released) {
            SDL_LogWarn(
                SDL_LOG_CATEGORY_INPUT,
                "[CAMERA] relative mouse release failed: %s",
                SDL_GetError()
            );
        }
        return released;
    }

    if (paused_ ||
        (SDL_GetWindowFlags(window_) & SDL_WINDOW_INPUT_FOCUS) == 0) {
        cameraDragging_ = false;
        return false;
    }

    if (!SDL_SetWindowRelativeMouseMode(window_, true)) {
        cameraDragging_ = false;
        SDL_LogWarn(
            SDL_LOG_CATEGORY_INPUT,
            "[CAMERA] relative mouse capture failed: %s",
            SDL_GetError()
        );
        return false;
    }

    cameraDragging_ = SDL_GetWindowRelativeMouseMode(window_);
    cameraCaptureWarmupFrames_ = cameraDragging_ ? 2 : 0;
    float discardedX = 0.0f;
    float discardedY = 0.0f;
    (void)SDL_GetRelativeMouseState(&discardedX, &discardedY);
    SDL_Log(
        cameraDragging_
            ? "[CAMERA] RMB orbit capture // ACTIVE"
            : "[CAMERA] RMB orbit capture // FAILED"
    );
    return cameraDragging_;
}

void HakuiApp::openGamepad(SDL_JoystickID instanceId)
{
    if (gamepad_) {
        return;
    }

    gamepad_ = SDL_OpenGamepad(instanceId);
    if (gamepad_) {
        inputBridge_.noteGamepadOpened();
        showInputStatus("GAMEPAD SYNCHRONIZED // RIDE INPUT AVAILABLE");
        SDL_Log("[HAKUI] controller online // %s", SDL_GetGamepadName(gamepad_));
        recordObserverEvent("input", "gamepad connected");
    }
}

SDL_AppResult HakuiApp::handleEvent(const SDL_Event& event)
{
    inputBridge_.observeEvent(event);

    if (event.type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    if (event.type == SDL_EVENT_GAMEPAD_ADDED) {
        openGamepad(event.gdevice.which);
    }

    if (event.type == SDL_EVENT_GAMEPAD_REMOVED && gamepad_ &&
        SDL_GetGamepadID(gamepad_) == event.gdevice.which) {
        SDL_CloseGamepad(gamepad_);
        gamepad_ = nullptr;
        inputBridge_.noteGamepadClosed();
        rideControls_.reset();
        rideControlFrame_ = {};
        SDL_Log("[HAKUI] controller offline");
        recordObserverEvent("input", "gamepad disconnected");
    }

    if (!paused_ && event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
        event.button.button == SDL_BUTTON_RIGHT &&
        event.button.windowID == SDL_GetWindowID(window_)) {
        (void)setCameraCapture(true);
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
        event.button.button == SDL_BUTTON_RIGHT) {
        (void)setCameraCapture(false);
    }

    if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST) {
        (void)setCameraCapture(false);
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult HakuiApp::tick()
{
    const Uint64 now = SDL_GetPerformanceCounter();
    const Uint64 frequency = SDL_GetPerformanceFrequency();
    float dt = static_cast<float>(now - previousCounter_) /
               static_cast<float>(frequency);
    previousCounter_ = now;

    // Avoid simulation explosions after a debugger pause or window stall.
    dt = std::min(dt, 0.1f);

    update(dt);
    if (quitRequested_) {
        return SDL_APP_SUCCESS;
    }
    if (!render()) {
        return SDL_APP_FAILURE;
    }
    if (expertCaptureRequested_) {
        expertCaptureRequested_ = false;
        if (!SDL_WaitForGPUIdle(gpu_)) {
            SDL_LogWarn(
                SDL_LOG_CATEGORY_APPLICATION,
                "[OBSERVER] GPU wait failed before frame capture: %s",
                SDL_GetError()
            );
        }
        captureExpertSnapshot();
    }
    return SDL_APP_CONTINUE;
}

void HakuiApp::updateHud()
{
    using hakui::input::Action;
    using hakui::input::InputResolver;

    const char* aumPhase = "A";
    switch (spiral_.aumField().phase()) {
        case spiral::AUMPhase::A_Emergence: aumPhase = "A"; break;
        case spiral::AUMPhase::U_Sustain:   aumPhase = "U"; break;
        case spiral::AUMPhase::M_Return:    aumPhase = "M"; break;
    }

    const auto prompt = [&](Action action) {
        return InputResolver::prompt(
            action,
            inputFrame_.activeDevice,
            inputFrame_.controllerLayout
        );
    };
    const std::string_view device =
        InputResolver::deviceName(inputFrame_.activeDevice);
    const auto jump = prompt(Action::Jump);
    const auto interact = prompt(Action::Interact);
    const auto cancel = prompt(Action::Cancel);
    const auto primary = prompt(Action::PrimaryAction);
    const auto secondary = prompt(Action::SecondaryAction);
    const auto guard = prompt(Action::Guard);
    const auto recover = prompt(Action::Recover);
    const auto pause = prompt(Action::Pause);
    const auto orbit = prompt(Action::OrbitCamera);
    const auto grind = prompt(Action::Grind);
    const auto balance = prompt(Action::Balance);
    const auto accelerate = prompt(Action::Accelerate);
    const auto spinLeft = prompt(Action::SpinLeft);
    const auto spinRight = prompt(Action::SpinRight);

    char title[768];
    if (inputStatusTimer_ > 0.0f && !inputStatus_.empty()) {
        SDL_snprintf(
            title,
            sizeof(title),
            "HAKUI v0.84 // %s // INPUT %.*s",
            inputStatus_.c_str(),
            static_cast<int>(device.size()), device.data()
        );
    } else if (paused_) {
        SDL_snprintf(
            title,
            sizeof(title),
            "HAKUI v0.84 // PAUSED // %.*s RESUME // LOOK %.4f // AUDIO %.0f%% // INPUT %.*s // MOUSE RELEASED",
            static_cast<int>(pause.size()), pause.data(),
            debugRenderer_.lookSensitivity(),
            audio_.volume() * 100.0f,
            static_cast<int>(device.size()), device.data()
        );
    } else if (combat_.active()) {
        SDL_snprintf(
            title,
            sizeof(title),
            "HAKUI v0.84 // SPAR %s // HP %.0f STA %.0f BAL %.0f // TARGET HP %.0f BAL %.0f // MOVE FOOTWORK // %.*s PRIMARY // %.*s SECONDARY // %.*s GUARD // %.*s RECOVER // %.*s LEAVE // %.*s",
            combatStateLabel(combat_.player().state),
            combat_.player().health,
            combat_.player().stamina,
            combat_.player().balance,
            combat_.opponent().health,
            combat_.opponent().balance,
            static_cast<int>(primary.size()), primary.data(),
            static_cast<int>(secondary.size()), secondary.data(),
            static_cast<int>(guard.size()), guard.data(),
            static_cast<int>(recover.size()), recover.data(),
            static_cast<int>(cancel.size()), cancel.data(),
            static_cast<int>(device.size()), device.data()
        );
    } else if (player_.activity == PlayerActivity::CasinoSeated && terminal_) {
        const std::string_view contextAction =
            hakui::games::GameTerminal::contextActionLabel(
                terminal_->nextContextAction()
            );
        const int handValue = hakui::games::BlackjackTable::handValue(
            terminal_->cardTable().playerHand()
        );
        SDL_snprintf(
            title,
            sizeof(title),
            "HAKUI v0.84 // FUSION TABLE // CREDITS %lld // HAND %d // %.*s %.*s // %.*s LEAVE // CONTEXTUAL INTERACTION // %.*s",
            static_cast<long long>(terminal_->virtualCredits()),
            handValue,
            static_cast<int>(interact.size()), interact.data(),
            static_cast<int>(contextAction.size()), contextAction.data(),
            static_cast<int>(cancel.size()), cancel.data(),
            static_cast<int>(device.size()), device.data()
        );
    } else if (player_.activity == PlayerActivity::CouchSeated) {
        SDL_snprintf(
            title,
            sizeof(title),
            "HAKUI v0.84 // VOID COUCH // AUM %s // %.*s STAND // %.*s ORBIT // %.*s PAUSE // %.*s",
            aumPhase,
            static_cast<int>(interact.size()), interact.data(),
            static_cast<int>(orbit.size()), orbit.data(),
            static_cast<int>(pause.size()), pause.data(),
            static_cast<int>(device.size()), device.data()
        );
    } else if (player_.y < -0.45f) {
        SDL_snprintf(
            title,
            sizeof(title),
            "HAKUI v0.84 // BLACK SPACE FALL // DEPTH %.1f // RECOVERY ARMED // RESPAWNS %u // %.*s",
            -player_.y,
            player_.voidRespawns,
            static_cast<int>(device.size()), device.data()
        );
    } else if (player_.locomotion == LocomotionMode::Skateboard ||
               player_.locomotion == LocomotionMode::BMX) {
        const hakui::RideableState& ride = rideable_.state();
        char combo[180]{};
        std::size_t offset = 0;
        for (std::size_t index = 0; index < ride.comboCount &&
             offset + 4 < sizeof(combo); ++index) {
            const std::string_view label =
                hakui::RideableMovementController::trickLabel(ride.combo[index]);
            const int written = SDL_snprintf(
                combo + offset,
                sizeof(combo) - offset,
                "%s%.*s",
                index == 0 ? "" : " > ",
                static_cast<int>(label.size()),
                label.data()
            );
            if (written <= 0) {
                break;
            }
            offset = std::min(
                sizeof(combo) - 1,
                offset + static_cast<std::size_t>(written)
            );
        }
        const std::string_view trick =
            hakui::RideableMovementController::trickLabel(ride.activeTrick);
        const std::string_view phase =
            hakui::RideableMovementController::phaseLabel(ride.phase);
        const std::string_view landing =
            hakui::RideableMovementController::landingLabel(ride.landingQuality);
        SDL_snprintf(
            title,
            sizeof(title),
            "HAKUI v0.84 // %s // %.*s // %.*s // SPD %.1f BAL %.0f // LAND %.*s // COMBO %s // %.*s TAP POP / HOLD PRELOAD // THEN RS FLICK AIR TRICK // %.*s GRIND // %.*s BALANCE // %.*s DRIVE // %.*s/%.*s SPIN // %.*s STYLE // %.*s DISMOUNT // %.*s",
            locomotionLabel(player_.locomotion),
            static_cast<int>(phase.size()), phase.data(),
            static_cast<int>(trick.size()), trick.data(),
            ride.speed,
            ride.balance,
            static_cast<int>(landing.size()), landing.data(),
            ride.comboCount > 0 ? combo : "READY",
            static_cast<int>(jump.size()), jump.data(),
            static_cast<int>(grind.size()), grind.data(),
            static_cast<int>(balance.size()), balance.data(),
            static_cast<int>(accelerate.size()), accelerate.data(),
            static_cast<int>(spinLeft.size()), spinLeft.data(),
            static_cast<int>(spinRight.size()), spinRight.data(),
            static_cast<int>(primary.size()), primary.data(),
            static_cast<int>(cancel.size()), cancel.data(),
            static_cast<int>(device.size()), device.data()
        );
    } else {
        const hakui::RoomInteractionFocus focus = blackRoom_.nearestInteraction(player_);
        if (focus) {
            SDL_snprintf(
                title,
                sizeof(title),
                "HAKUI v0.84 // %.*s // %.*s USE // %.*s ORBIT // MODE %s // STAMINA %.0f // AUM %s // %.*s",
                static_cast<int>(focus.prompt.size()),
                focus.prompt.data(),
                static_cast<int>(interact.size()), interact.data(),
                static_cast<int>(orbit.size()), orbit.data(),
                locomotionLabel(player_.locomotion),
                player_.stamina,
                aumPhase,
                static_cast<int>(device.size()), device.data()
            );
        } else if (blackRoom_.hasAffordanceAt(
                       hakui::WorldAffordance::FightZone,
                       player_.x,
                       player_.y,
                       player_.z)) {
            SDL_snprintf(
                title,
                sizeof(title),
                "HAKUI v0.84 // SPARRING DATUM // %.*s ENTER // DUMMY VISIBLE // %.*s/%.*s ATTACK // %.*s GUARD // %.*s RECOVER // %.*s",
                static_cast<int>(cancel.size()), cancel.data(),
                static_cast<int>(primary.size()), primary.data(),
                static_cast<int>(secondary.size()), secondary.data(),
                static_cast<int>(guard.size()), guard.data(),
                static_cast<int>(recover.size()), recover.data(),
                static_cast<int>(device.size()), device.data()
            );
        } else {
            SDL_snprintf(
                title,
                sizeof(title),
                "HAKUI v0.84 // %s // X %.1f Y %.1f Z %.1f // STA %.0f // CAM %.2f/%.2f/%.1f // ORBIT %s // INTENT DEVICE %.*s // AUM %s",
                locomotionLabel(player_.locomotion),
                player_.x,
                player_.y,
                player_.z,
                player_.stamina,
                debugRenderer_.cameraYaw(),
                debugRenderer_.cameraPitch(),
                debugRenderer_.cameraDistance(),
                cameraDragging_ ? "ACTIVE" : "READY",
                static_cast<int>(device.size()), device.data(),
                aumPhase
            );
        }
    }
    SDL_SetWindowTitle(window_, title);
}

void HakuiApp::update(float dt)
{
    using hakui::input::Action;
    using hakui::input::Axis;

    inputFrame_ = inputBridge_.sample(gamepad_, dt, cameraDragging_);
    inputStatusTimer_ = std::max(0.0f, inputStatusTimer_ - dt);

    if (inputFrame_.action(Action::CaptureExpertSnapshot).pressed) {
        expertCaptureRequested_ = true;
        recordObserverEvent("observer.request", "F12 snapshot armed");
        showInputStatus("EXPERT SNAPSHOT // CAPTURE ARMED", 2.0f);
    }

    if (inputFrame_.gamepadConnected) {
        showInputStatus("GAMEPAD SYNCHRONIZED // PROMPTS RECALIBRATED");
    }
    if (inputFrame_.gamepadDisconnected) {
        if (player_.locomotion == LocomotionMode::Skateboard ||
            player_.locomotion == LocomotionMode::BMX) {
            switchLocomotion(LocomotionMode::OnFoot, "on_foot");
            showInputStatus(
                "RIDE INPUT LOST // RETURNED TO ON FOOT",
                4.0f
            );
        } else {
            showInputStatus("GAMEPAD OFFLINE // KEYBOARD + MOUSE RESTORED");
        }
    }

    if (inputFrame_.action(Action::Pause).pressed) {
        setPaused(!paused_);
    }
    if (inputFrame_.action(Action::Quit).pressed) {
        quitRequested_ = true;
    }
    if (inputFrame_.action(Action::LookSlower).pressed) {
        debugRenderer_.adjustLookSensitivity(-0.0005f);
    }
    if (inputFrame_.action(Action::LookFaster).pressed) {
        debugRenderer_.adjustLookSensitivity(0.0005f);
    }
    if (inputFrame_.action(Action::VolumeDown).pressed) {
        audio_.adjustVolume(-0.05f);
    }
    if (inputFrame_.action(Action::VolumeUp).pressed) {
        audio_.adjustVolume(0.05f);
    }
    if (inputFrame_.action(Action::ToggleRideFallback).pressed) {
        developerRideFallback_ = !developerRideFallback_;
        showInputStatus(
            developerRideFallback_
                ? "DEVELOPER RIDE FALLBACK // ENABLED"
                : "DEVELOPER RIDE FALLBACK // DISABLED"
        );
    }

    if (!paused_) {
        world_.elapsedSeconds += dt;

        const bool rideActiveAtInput =
            player_.locomotion == LocomotionMode::Skateboard ||
            player_.locomotion == LocomotionMode::BMX;
        rideControlFrame_ = rideControls_.update(
            inputFrame_,
            rideActiveAtInput,
            rideActiveAtInput && !player_.grounded,
            dt
        );

        // Spiral advances as part of the same native client heartbeat, but its
        // internals remain platform-independent.
        spiral_.tick(dt);

        if (!combat_.active() &&
            inputFrame_.action(Action::CameraReset).pressed) {
            debugRenderer_.resetCamera();
        }
        const float zoom = inputFrame_.axis(Axis::Zoom);
        if (zoom != 0.0f) {
            debugRenderer_.zoomCamera(zoom);
        }

        float lookRight = inputFrame_.axis(Axis::LookRight);
        float lookDown = inputFrame_.axis(Axis::LookDown);
        if (inputFrame_.activeDevice == hakui::input::InputDevice::Gamepad) {
            lookRight = 0.0f;
            lookDown = 0.0f;
            if (rideControlFrame_.rightStickOwner ==
                hakui::input::RightStickOwner::Camera) {
                hakui::input::RideControlInterpreter::radialDeadzone(
                    inputFrame_.axis(Axis::RightStickX),
                    inputFrame_.axis(Axis::RightStickY),
                    rideControls_.tuning().cameraDeadzone,
                    lookRight,
                    lookDown
                );
                lookRight *= 520.0f * dt;
                lookDown *= 420.0f * dt;
            }
        }
        if (cameraCaptureWarmupFrames_ > 0 &&
            inputFrame_.activeDevice == hakui::input::InputDevice::KeyboardMouse) {
            --cameraCaptureWarmupFrames_;
            lookRight = 0.0f;
            lookDown = 0.0f;
        }
        constexpr float kMaximumCredibleMouseDelta = 200.0f;
        if (inputFrame_.activeDevice == hakui::input::InputDevice::KeyboardMouse &&
            (std::fabs(lookRight) > kMaximumCredibleMouseDelta ||
             std::fabs(lookDown) > kMaximumCredibleMouseDelta)) {
            lookRight = 0.0f;
            lookDown = 0.0f;
        }
        if (std::isfinite(lookRight) && std::isfinite(lookDown) &&
            (lookRight != 0.0f || lookDown != 0.0f)) {
            debugRenderer_.orbitCamera(lookRight, lookDown);
        }

        if (inputFrame_.action(Action::Cancel).pressed) {
            if (combat_.active()) {
                toggleCombat();
            } else if (player_.activity != PlayerActivity::Roaming) {
                leaveCurrentInteraction();
            } else if (player_.locomotion == LocomotionMode::Skateboard ||
                       player_.locomotion == LocomotionMode::BMX) {
                switchLocomotion(LocomotionMode::OnFoot, "on_foot");
            } else if (blackRoom_.hasAffordanceAt(
                           hakui::WorldAffordance::FightZone,
                           player_.x,
                           player_.y,
                           player_.z)) {
                toggleCombat();
            }
        }

        const bool riding = player_.locomotion == LocomotionMode::Skateboard ||
            player_.locomotion == LocomotionMode::BMX;
        if (!combat_.active() && !riding &&
            inputFrame_.action(Action::Interact).pressed) {
            handlePrimaryInteraction();
        }

        if (inputFrame_.action(Action::SelectOnFoot).pressed &&
            player_.activity == PlayerActivity::Roaming && !combat_.active()) {
            switchLocomotion(LocomotionMode::OnFoot, "on_foot");
        }
        if (inputFrame_.action(Action::SelectSkateboard).pressed &&
            player_.activity == PlayerActivity::Roaming && !combat_.active()) {
            requestRideLocomotion(LocomotionMode::Skateboard, "skateboard");
        }
        if (inputFrame_.action(Action::SelectBmx).pressed &&
            player_.activity == PlayerActivity::Roaming && !combat_.active()) {
            requestRideLocomotion(LocomotionMode::BMX, "bmx");
        }
        if (inputFrame_.action(Action::SelectCar).pressed &&
            player_.activity == PlayerActivity::Roaming && !combat_.active()) {
            switchLocomotion(LocomotionMode::Car, "car");
        }

        hakui::input::ActiveDiscipline discipline =
            hakui::input::ActiveDiscipline::OnFoot;
        if (combat_.active()) {
            discipline = hakui::input::ActiveDiscipline::Boxing;
        } else if (player_.activity != PlayerActivity::Roaming) {
            discipline = hakui::input::ActiveDiscipline::Seated;
        } else if (player_.locomotion == LocomotionMode::Skateboard) {
            discipline = hakui::input::ActiveDiscipline::Skateboard;
        } else if (player_.locomotion == LocomotionMode::BMX) {
            discipline = hakui::input::ActiveDiscipline::Bmx;
        }
        const hakui::input::DisciplineIntent embodimentIntent =
            hakui::input::DisciplineInterpreter::interpret(
                inputFrame_,
                discipline
            );

        const float inputRight = embodimentIntent.moveRight;
        const float inputForward = embodimentIntent.moveForward;
        const hakui::math::Vec3 cameraMovement =
            hakui::math::cameraRelativePlanarMovement(
                inputRight,
                inputForward,
                debugRenderer_.movementYaw()
            );

        hakui::MovementInput movementInput;
        const bool rideActive = player_.locomotion == LocomotionMode::Skateboard ||
            player_.locomotion == LocomotionMode::BMX;
        if (rideActive) {
            const float moveMagnitude = std::min(
                1.0f,
                std::sqrt(inputRight * inputRight + inputForward * inputForward)
            );
            const float propulsion = std::max(
                moveMagnitude,
                rideControlFrame_.propulsionIntent
            );
            if (moveMagnitude > 0.15f) {
                const float scale = propulsion / moveMagnitude;
                movementInput.right = cameraMovement.x * scale;
                movementInput.forward = cameraMovement.z * scale;
            } else if (propulsion > 0.0f) {
                movementInput.right = std::sin(player_.yaw) * propulsion;
                movementInput.forward = std::cos(player_.yaw) * propulsion;
            }
            movementInput.sprint = rideControlFrame_.propulsionIntent > 0.20f;
        } else {
            movementInput.right = cameraMovement.x;
            movementInput.forward = cameraMovement.z;
            movementInput.sprint = embodimentIntent.accelerate > 0.20f;
        }
        movementInput.jumpPressed =
            embodimentIntent.traversal == hakui::input::TraversalIntent::Jump;

        hakui::MovementStep movementStep;
        if (combat_.active()) {
            player_.x = combat_.zone().playerAnchor.x;
            player_.y = combat_.zone().playerAnchor.y;
            player_.z = combat_.zone().playerAnchor.z;
            const hakui::WorldAffordanceVolume* activeZone =
                blackRoom_.affordanceById(combat_.zone().worldAffordanceId);
            player_.yaw = activeZone
                ? activeZone->primaryAnchor.yaw
                : 1.57079632679f;
            player_.velocityX = 0.0f;
            player_.velocityY = 0.0f;
            player_.velocityZ = 0.0f;
            player_.grounded = true;
            updateCombat(dt, embodimentIntent);
        } else if (player_.locomotion == LocomotionMode::Skateboard ||
                   player_.locomotion == LocomotionMode::BMX) {
            hakui::RideableInput rideInput;
            rideInput.movement = movementInput;
            rideInput.movement.jumpPressed = false;
            rideInput.popPressed = rideControlFrame_.popIntent;
            rideInput.popPreload = rideControlFrame_.popPreload;
            rideInput.manualHeld = rideControlFrame_.balanceIntent;
            rideInput.grindHeld = rideControlFrame_.grindIntent;
            rideInput.stylePressed = rideControlFrame_.styleIntent;
            rideInput.spinLeft = rideControlFrame_.spinLeftIntent;
            rideInput.spinRight = rideControlFrame_.spinRightIntent;
            rideInput.propulsion = rideControlFrame_.propulsionIntent;
            rideInput.trick = {
                rideTrickDirection(rideControlFrame_.trick.direction),
                rideControlFrame_.trick.normalizedX,
                rideControlFrame_.trick.normalizedY,
                rideControlFrame_.trick.magnitude,
                rideControlFrame_.trick.gestureDuration,
                rideControlFrame_.trickIntent
            };
            const hakui::RideableFrame rideFrame = rideable_.update(
                player_,
                rideInput,
                blackRoom_.movementEnvironment(),
                blackRoom_.affordances(),
                dt
            );
            movementStep = rideFrame.movement;
            if (rideFrame.movement.jumped) {
                rideControls_.armTrickWindow();
                rideControlFrame_ = rideControls_.diagnostics();
            }
            if (rideFrame.landed || rideFrame.bailed) {
                rideControls_.closeTrickWindow();
                rideControlFrame_ = rideControls_.diagnostics();
            }
            if (rideFrame.trickStarted) {
                const std::string_view trick =
                    hakui::RideableMovementController::trickLabel(
                        rideable_.state().activeTrick
                    );
                SDL_Log(
                    "[RIDE] %.*s",
                    static_cast<int>(trick.size()),
                    trick.data()
                );
                if (rideable_.state().rotationChannel ==
                    hakui::RideRotationChannel::BoardDeck) {
                    audio_.play(HakuiAudioCue::BoardRotation);
                } else if (rideable_.state().rotationChannel !=
                           hakui::RideRotationChannel::None) {
                    audio_.play(HakuiAudioCue::BmxTrick);
                }
            }
            if (rideFrame.grindStarted) {
                audio_.play(HakuiAudioCue::GrindScrape);
            }
            if (rideFrame.bailed) {
                audio_.play(HakuiAudioCue::BailImpact);
                SDL_Log("[RIDE] BAIL // combo interrupted");
            }
        } else {
            movementStep = movement_.update(
                player_,
                movementInput,
                blackRoom_.movementEnvironment(),
                dt
            );
        }
        if (movementStep.jumped) {
            audio_.play(riding ? HakuiAudioCue::RidePop : HakuiAudioCue::Jump);
        }
        if (movementStep.landed) {
            if (!riding) {
                audio_.play(HakuiAudioCue::Land);
            } else if (rideable_.state().landingQuality ==
                       hakui::LandingQuality::Clean) {
                audio_.play(HakuiAudioCue::CleanLanding);
            } else if (rideable_.state().landingQuality ==
                       hakui::LandingQuality::Sketchy) {
                audio_.play(HakuiAudioCue::SketchyLanding);
            }
        }
        if (movementStep.respawned) {
            rideControls_.reset();
            rideControlFrame_ = {};
            audio_.play(HakuiAudioCue::VoidRespawn);
            SDL_Log("[HAKUI] BLACK SPACE recovery // respawn %u", player_.voidRespawns);
            recordObserverEvent("world", "black space respawn");
        }

        player_.sprinting = movementStep.sprinting;
        const float targetMovementBlend = movementStep.moved && player_.grounded
            ? (movementStep.sprinting ? 1.0f : 0.62f)
            : 0.0f;
        const float blendResponse = 1.0f - std::exp(-12.0f * dt);
        player_.movementBlend +=
            (targetMovementBlend - player_.movementBlend) * blendResponse;
        player_.idlePhase += 1.8f * dt;
        if (movementStep.moved && player_.grounded) {
            const float gaitSpeed = movementStep.sprinting ? 11.0f : 7.2f;
            player_.gaitPhase += gaitSpeed * dt;
            footstepDistance_ += movementStep.distance;
            const float footstepSpacing = movementStep.sprinting ? 1.05f : 0.78f;
            if (footstepDistance_ >= footstepSpacing) {
                footstepDistance_ = std::fmod(footstepDistance_, footstepSpacing);
                alternateFootstep_ = !alternateFootstep_;
                audio_.play(
                    alternateFootstep_
                        ? HakuiAudioCue::FootstepSoft
                        : HakuiAudioCue::FootstepHard
                );
            }
        }

        locomotion_.update(dt);
    }

    debugRenderer_.updateCamera(dt, player_);

    titleTimer_ += dt;
    if (titleTimer_ >= 0.10f) {
        titleTimer_ = 0.0f;
        updateHud();
    }
}

bool HakuiApp::render()
{
    SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(gpu_);
    if (!commands) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "[HAKUI] command buffer failed: %s",
            SDL_GetError()
        );
        return false;
    }

    SDL_GPUTexture* swapchain = nullptr;
    Uint32 width = 0;
    Uint32 height = 0;

    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            commands,
            window_,
            &swapchain,
            &width,
            &height)) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "[HAKUI] swapchain acquire failed: %s",
            SDL_GetError()
        );
        SDL_CancelGPUCommandBuffer(commands);
        return false;
    }

    if (!swapchain) {
        SDL_CancelGPUCommandBuffer(commands);
        return true;
    }

    HakuiSceneState scene;
    scene.paused = paused_;
    scene.rideable = rideable_.state();
    scene.combatActive = combat_.active();
    const hakui::WorldAffordanceVolume* sparZone =
        blackRoom_.firstAffordance(hakui::WorldAffordance::FightZone);
    scene.sparDummyVisible = sparZone != nullptr;
    if (sparZone) {
        scene.opponentX = sparZone->secondaryAnchor.x;
        scene.opponentY = sparZone->secondaryAnchor.y;
        scene.opponentZ = sparZone->secondaryAnchor.z;
        scene.opponentYaw = sparZone->secondaryAnchor.yaw;
    }
    if (scene.combatActive) {
        scene.playerCombatState = combat_.player().state;
        scene.opponentCombatState = combat_.opponent().state;
        scene.playerAttack = combat_.player().pendingAttack;
        scene.opponentAttack = combat_.opponent().pendingAttack;
        scene.opponentX = combat_.opponentWorldPosition().x;
        scene.opponentY = combat_.opponentWorldPosition().y;
        scene.opponentZ = combat_.opponentWorldPosition().z;
        scene.opponentYaw = std::atan2(
            player_.x - scene.opponentX,
            player_.z - scene.opponentZ
        );
        scene.playerHitPulse = playerHitPulse_;
        scene.opponentHitPulse = opponentHitPulse_;
        scene.playerStanceBlend = combat_.player().stanceBlend;
        scene.opponentStanceBlend = combat_.opponent().stanceBlend;
    }
    if (terminal_) {
        scene.terminalPowered = terminal_->powered();
        scene.cardSuiteActive =
            terminal_->activeApp() == hakui::games::TerminalApp::CardTable52;
        scene.diceTotal = terminal_->lastDiceResult().total;
    }

    if (!debugRenderer_.render(
            commands,
            swapchain,
            width,
            height,
            player_,
            scene,
            blackRoom_.geometry())) {
        SDL_LogError(
            SDL_LOG_CATEGORY_APPLICATION,
            "[HAKUI] world render failed: %s",
            SDL_GetError()
        );
        SDL_CancelGPUCommandBuffer(commands);
        return false;
    }

    return SDL_SubmitGPUCommandBuffer(commands);
}

void HakuiApp::shutdown()
{
    SDL_Log("[HAKUI] shutting down");

    if (window_) {
        (void)setCameraCapture(false);
    }

    // Optional capability modules detach before platform/runtime teardown.
    spiral_.crystalHost().unmountAll();

    spiral::Signal shutdownSignal;
    shutdownSignal.kind = spiral::SignalKind::State;
    shutdownSignal.source = "hakui.client";
    shutdownSignal.destination = "spiral.core";
    shutdownSignal.topic = "client.shutdown";
    shutdownSignal.payload = "normal";
    shutdownSignal.statePatch = {
        {"client.status", std::string("offline")}
    };
    spiral_.dispatch(std::move(shutdownSignal));

    terminal_.reset();

    if (gamepad_) {
        SDL_CloseGamepad(gamepad_);
        gamepad_ = nullptr;
    }

    audio_.shutdown();

    if (spiralListener_ != 0) {
        spiral_.router().unsubscribe(spiralListener_);
        spiralListener_ = 0;
    }

    debugRenderer_.shutdown();

    if (gpu_ && window_) {
        SDL_ReleaseWindowFromGPUDevice(gpu_, window_);
    }

    if (gpu_) {
        SDL_DestroyGPUDevice(gpu_);
        gpu_ = nullptr;
    }

    if (window_) {
        SDL_DestroyWindow(window_);
        window_ = nullptr;
    }

    SDL_Quit();
}
