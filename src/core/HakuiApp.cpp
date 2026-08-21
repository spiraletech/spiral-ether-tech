#include "core/HakuiApp.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

#include "render/Math3D.hpp"

namespace {

float gamepadAxis(Sint16 raw)
{
    constexpr float deadZone = 0.16f;
    const float value = std::clamp(
        static_cast<float>(raw) / 32767.0f,
        -1.0f,
        1.0f
    );
    const float magnitude = std::fabs(value);
    if (magnitude <= deadZone) {
        return 0.0f;
    }
    const float remapped = (magnitude - deadZone) / (1.0f - deadZone);
    return std::copysign(remapped, value);
}

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

} // namespace

bool HakuiApp::boot()
{
    SDL_Log("[HAKUI] booting native client v0.7-dev // EMBODIMENT PASS");

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
    SDL_Log("[HAKUI] v0.7 movement // foot + skateboard + BMX embodiment online");
    SDL_Log("[HAKUI] procedural locomotion // idle + walk + sprint + jump + seated online");
    SDL_Log("[HAKUI] BLACK ROOM // neon lounge + couch + fusion table + open void");
    SDL_Log("[HAKUI] controls // WASD move // SPACE jump // E interact/stand // SHIFT sprint");
    SDL_Log("[HAKUI] camera // RMB orbit // WHEEL zoom // Q shoulder // R reset");
    SDL_Log("[HAKUI] menu // ESC pause // [ ] look sensitivity // - + audio // F10 quit");
    SDL_Log("[HAKUI] modes // 1 foot // 2 visible skateboard // 3 visible BMX // 4 car deferred");
    SDL_Log("[HAKUI] FUSION TABLE // T dice // G cards // B bet 25 // H hit // J stand // I inspect");
    SDL_Log("[HAKUI] tabletop input is locked until seated // virtual credits only");
    SDL_Log("[HAKUI] combat grammar // C enter/leave spar // Z jab // X cross // V guard // K recover");
    SDL_Log("[HAKUI] combat foundation // unarmed playable // sword + bow extension seams dormant");
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
        "SPIRAL OS: HAKUI ENGINE // v0.7-dev // EMBODIMENT PASS",
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
    bootSignal.payload = "hakui-v0.7-dev";

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
        {"client.version", std::string("0.7-dev")},
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
    locomotion_.switchTo(mode);

    if (mode == LocomotionMode::Car) {
        SDL_Log("[HAKUI] CAR // representation and controller deferred after v0.7");
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

void HakuiApp::handleCasinoCommand(SDL_Keycode key)
{
    if (player_.activity != PlayerActivity::CasinoSeated || !terminal_) {
        SDL_Log("[TABLETOP] control anchored to table // press E at the chair");
        return;
    }

    switch (key) {
        case SDLK_T:
            interactWithTerminal(hakui::InteractionVerb::Use);
            if (terminal_->lastDiceReward() > 0) {
                SDL_Log(
                    "[TABLETOP] doubles // earned %lld virtual credits",
                    static_cast<long long>(terminal_->lastDiceReward())
                );
            }
            break;

        case SDLK_G:
            interactWithTerminal(hakui::InteractionVerb::Play);
            break;

        case SDLK_I:
            interactWithTerminal(hakui::InteractionVerb::Inspect);
            break;

        case SDLK_B:
            if (terminal_->beginCardRound(25)) {
                SDL_Log("[TABLETOP] round started // virtual wager 25");
                audio_.play(HakuiAudioCue::Casino);
            } else {
                SDL_Log("[TABLETOP] press G for cards, then B to wager 25");
            }
            break;

        case SDLK_H:
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

        case SDLK_J:
            if (terminal_->standCardTable()) {
                SDL_Log(
                    "[TABLETOP] stand // virtual credits %lld",
                    static_cast<long long>(terminal_->virtualCredits())
                );
                audio_.play(HakuiAudioCue::Casino);
            }
            break;

        default:
            break;
    }
}

void HakuiApp::handlePrimaryInteraction()
{
    if (combat_.active()) {
        SDL_Log("[COMBAT] interaction locked during spar // C to leave");
        return;
    }

    if (player_.activity != PlayerActivity::Roaming) {
        if (blackRoom_.leaveInteraction(player_)) {
            debugRenderer_.setCameraRole(CameraRole::GameplayFollow);
            audio_.play(HakuiAudioCue::Interact);
            SDL_Log("[HAKUI] interaction released // movement restored");
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
        audio_.play(HakuiAudioCue::Casino);
    } else {
        debugRenderer_.frameInteraction(InteractionFrame::LoungeCouch);
        SDL_Log("[HAKUI] seated // VOID COUCH // E to stand");
        audio_.play(HakuiAudioCue::Interact);
    }
}

void HakuiApp::toggleCombat()
{
    if (combat_.active()) {
        const hakui::combat::CombatVector exitAnchor =
            combat_.zone().playerAnchor;
        combat_.leave();
        combatAttackQueued_ = hakui::combat::AttackSemantic::None;
        combatRecoverQueued_ = false;
        player_.x = exitAnchor.x - 0.90f;
        player_.y = exitAnchor.y;
        player_.z = exitAnchor.z;
        player_.yaw = 1.57079632679f;
        debugRenderer_.setCameraRole(CameraRole::GameplayFollow);
        audio_.play(HakuiAudioCue::Interact);
        SDL_Log("[COMBAT] spar released // free movement restored");
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
}

void HakuiApp::updateCombat(float dt, const bool* keys)
{
    if (!combat_.active()) {
        combatAttackQueued_ = hakui::combat::AttackSemantic::None;
        combatRecoverQueued_ = false;
        return;
    }

    using namespace hakui::combat;
    CombatIntent playerIntent;
    playerIntent.attack = combatAttackQueued_;
    playerIntent.recover = combatRecoverQueued_;
    playerIntent.defense = keys[SDL_SCANCODE_V]
        ? DefenseIntent::Guard
        : DefenseIntent::None;
    if (gamepad_ && SDL_GetGamepadButton(
            gamepad_,
            SDL_GAMEPAD_BUTTON_LEFT_SHOULDER)) {
        playerIntent.defense = DefenseIntent::Guard;
    }
    combatAttackQueued_ = AttackSemantic::None;
    combatRecoverQueued_ = false;

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
    if (frame.playerAttackStarted || frame.opponentAttackStarted) {
        audio_.play(HakuiAudioCue::CombatSwing);
    }
    for (std::size_t index = 0; index < frame.damageEventCount; ++index) {
        const DamageEvent& damage = frame.damageEvents[index];
        if (damage.result == HitResult::Miss) {
            continue;
        }
        if (damage.target == combat_.player().entity) {
            playerHitPulse_ = 0.16f;
        } else {
            opponentHitPulse_ = 0.16f;
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
    if (paused_) {
        (void)setCameraCapture(false);
    }
    SDL_Log(paused_ ? "[HAKUI] PAUSED" : "[HAKUI] RESUMED");
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

void HakuiApp::updateCameraOrbitInput()
{
    if (paused_ || !cameraDragging_) {
        return;
    }

    float deltaX = 0.0f;
    float deltaY = 0.0f;
    (void)SDL_GetRelativeMouseState(
        &deltaX,
        &deltaY
    );
    if (cameraCaptureWarmupFrames_ > 0) {
        --cameraCaptureWarmupFrames_;
        return;
    }
    constexpr float kMaximumCredibleRelativeDelta = 200.0f;
    if (std::fabs(deltaX) > kMaximumCredibleRelativeDelta ||
        std::fabs(deltaY) > kMaximumCredibleRelativeDelta) {
        SDL_LogDebug(
            SDL_LOG_CATEGORY_INPUT,
            "[CAMERA] discarded relative-mode transition delta %.1f/%.1f",
            deltaX,
            deltaY
        );
        return;
    }
    if (std::isfinite(deltaX) && std::isfinite(deltaY) &&
        (deltaX != 0.0f || deltaY != 0.0f)) {
        debugRenderer_.orbitCamera(deltaX, deltaY);
    }
}

void HakuiApp::openGamepad(SDL_JoystickID instanceId)
{
    if (gamepad_) {
        return;
    }

    gamepad_ = SDL_OpenGamepad(instanceId);
    if (gamepad_) {
        SDL_Log("[HAKUI] controller online // %s", SDL_GetGamepadName(gamepad_));
    }
}

void HakuiApp::handleGamepadButton(SDL_GamepadButton button)
{
    if (button == SDL_GAMEPAD_BUTTON_START) {
        setPaused(!paused_);
        return;
    }
    if (paused_) {
        return;
    }

    if (combat_.active()) {
        switch (button) {
            case SDL_GAMEPAD_BUTTON_SOUTH:
                combatAttackQueued_ = hakui::combat::AttackSemantic::Jab;
                break;
            case SDL_GAMEPAD_BUTTON_WEST:
                combatAttackQueued_ = hakui::combat::AttackSemantic::Cross;
                break;
            case SDL_GAMEPAD_BUTTON_NORTH:
                combatRecoverQueued_ = true;
                break;
            case SDL_GAMEPAD_BUTTON_EAST:
                toggleCombat();
                break;
            case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
                debugRenderer_.toggleShoulder();
                break;
            case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
                debugRenderer_.resetCamera();
                break;
            default:
                break;
        }
        return;
    }

    switch (button) {
        case SDL_GAMEPAD_BUTTON_WEST:
            handlePrimaryInteraction();
            break;
        case SDL_GAMEPAD_BUTTON_EAST:
            if (player_.activity != PlayerActivity::Roaming) {
                handlePrimaryInteraction();
            }
            break;
        case SDL_GAMEPAD_BUTTON_SOUTH:
            if (player_.activity == PlayerActivity::CasinoSeated) {
                handleCasinoCommand(SDLK_H);
            } else {
                jumpQueued_ = true;
            }
            break;
        case SDL_GAMEPAD_BUTTON_NORTH:
            handleCasinoCommand(SDLK_J);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_UP:
            handleCasinoCommand(SDLK_T);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
            handleCasinoCommand(SDLK_G);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
            handleCasinoCommand(SDLK_B);
            break;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
            toggleCombat();
            break;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
            debugRenderer_.toggleShoulder();
            break;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK:
            debugRenderer_.resetCamera();
            break;
        default:
            break;
    }
}

SDL_AppResult HakuiApp::handleEvent(const SDL_Event& event)
{
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
        SDL_Log("[HAKUI] controller offline");
    }

    if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        handleGamepadButton(static_cast<SDL_GamepadButton>(event.gbutton.button));
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

    if (!paused_ && event.type == SDL_EVENT_MOUSE_WHEEL) {
        const float wheelDirection =
            event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f;
        debugRenderer_.zoomCamera(event.wheel.y * wheelDirection);
    }

    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
        switch (event.key.key) {
            case SDLK_ESCAPE:
                setPaused(!paused_);
                break;

            case SDLK_F10:
                return SDL_APP_SUCCESS;

            case SDLK_LEFTBRACKET:
                debugRenderer_.adjustLookSensitivity(-0.0005f);
                break;

            case SDLK_RIGHTBRACKET:
                debugRenderer_.adjustLookSensitivity(0.0005f);
                break;

            case SDLK_MINUS:
                audio_.adjustVolume(-0.05f);
                break;

            case SDLK_EQUALS:
                audio_.adjustVolume(0.05f);
                break;

            default:
                break;
        }

        if (paused_) {
            return SDL_APP_CONTINUE;
        }

        switch (event.key.key) {
            case SDLK_C:
                toggleCombat();
                break;

            case SDLK_Z:
                if (combat_.active()) {
                    combatAttackQueued_ = hakui::combat::AttackSemantic::Jab;
                }
                break;

            case SDLK_X:
                if (combat_.active()) {
                    combatAttackQueued_ = hakui::combat::AttackSemantic::Cross;
                }
                break;

            case SDLK_K:
                if (combat_.active()) {
                    combatRecoverQueued_ = true;
                }
                break;

            case SDLK_Q:
                debugRenderer_.toggleShoulder();
                break;

            case SDLK_R:
                debugRenderer_.resetCamera();
                break;

            case SDLK_1:
                if (player_.activity == PlayerActivity::Roaming && !combat_.active()) {
                    switchLocomotion(LocomotionMode::OnFoot, "on_foot");
                }
                break;

            case SDLK_2:
                if (player_.activity == PlayerActivity::Roaming && !combat_.active()) {
                    switchLocomotion(LocomotionMode::Skateboard, "skateboard");
                }
                break;

            case SDLK_3:
                if (player_.activity == PlayerActivity::Roaming && !combat_.active()) {
                    switchLocomotion(LocomotionMode::BMX, "bmx");
                }
                break;

            case SDLK_4:
                if (player_.activity == PlayerActivity::Roaming && !combat_.active()) {
                    switchLocomotion(LocomotionMode::Car, "car");
                }
                break;

            case SDLK_SPACE:
                if (!combat_.active()) {
                    jumpQueued_ = true;
                }
                break;

            case SDLK_E:
                handlePrimaryInteraction();
                break;

            case SDLK_T:
            case SDLK_G:
            case SDLK_I:
            case SDLK_B:
            case SDLK_H:
            case SDLK_J:
                handleCasinoCommand(event.key.key);
                break;

            default:
                break;
        }
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
    return render() ? SDL_APP_CONTINUE : SDL_APP_FAILURE;
}

void HakuiApp::updateHud()
{
    const char* aumPhase = "A";
    switch (spiral_.aumField().phase()) {
        case spiral::AUMPhase::A_Emergence: aumPhase = "A"; break;
        case spiral::AUMPhase::U_Sustain:   aumPhase = "U"; break;
        case spiral::AUMPhase::M_Return:    aumPhase = "M"; break;
    }

    char title[512];
    if (paused_) {
        SDL_snprintf(
            title,
            sizeof(title),
            "HAKUI v0.7 // PAUSED // ESC RESUME // F10 QUIT // LOOK %.4f // AUDIO %.0f%% // MOUSE RELEASED",
            debugRenderer_.lookSensitivity(),
            audio_.volume() * 100.0f
        );
    } else if (combat_.active()) {
        SDL_snprintf(
            title,
            sizeof(title),
            "HAKUI v0.7 // SPAR %s // HP %.0f STA %.0f BAL %.0f // TARGET HP %.0f BAL %.0f // Z JAB // X CROSS // V GUARD // K RECOVER // C LEAVE",
            combatStateLabel(combat_.player().state),
            combat_.player().health,
            combat_.player().stamina,
            combat_.player().balance,
            combat_.opponent().health,
            combat_.opponent().balance
        );
    } else if (player_.activity == PlayerActivity::CasinoSeated && terminal_) {
        const int handValue = hakui::games::BlackjackTable::handValue(
            terminal_->cardTable().playerHand()
        );
        SDL_snprintf(
            title,
            sizeof(title),
            "HAKUI v0.7 // FUSION TABLE // CREDITS %lld // HAND %d // DICE %d // T ROLL // G CARDS // B BET // H HIT // J STAND // E LEAVE",
            static_cast<long long>(terminal_->virtualCredits()),
            handValue,
            terminal_->lastDiceResult().total
        );
    } else if (player_.activity == PlayerActivity::CouchSeated) {
        SDL_snprintf(
            title,
            sizeof(title),
            "HAKUI v0.7 // VOID COUCH // AUM %s // E STAND // RMB ORBIT // Q SHOULDER // ESC PAUSE",
            aumPhase
        );
    } else if (player_.y < -0.45f) {
        SDL_snprintf(
            title,
            sizeof(title),
            "HAKUI v0.7 // BLACK SPACE FALL // DEPTH %.1f // RECOVERY ARMED // RESPAWNS %u",
            -player_.y,
            player_.voidRespawns
        );
    } else {
        const hakui::RoomInteractionFocus focus = blackRoom_.nearestInteraction(player_);
        if (focus) {
            SDL_snprintf(
                title,
                sizeof(title),
                "HAKUI v0.7 // %.*s // E USE // RMB ORBIT // MODE %s // STAMINA %.0f // AUM %s",
                static_cast<int>(focus.prompt.size()),
                focus.prompt.data(),
                locomotionLabel(player_.locomotion),
                player_.stamina,
                aumPhase
            );
        } else if (blackRoom_.hasAffordanceAt(
                       hakui::WorldAffordance::FightZone,
                       player_.x,
                       player_.y,
                       player_.z)) {
            SDL_snprintf(
                title,
                sizeof(title),
                "HAKUI v0.7 // SPARRING DATUM // C ENTER // DUMMY VISIBLE // Z/X ATTACK // V GUARD // K RECOVER"
            );
        } else {
            SDL_snprintf(
                title,
                sizeof(title),
                "HAKUI v0.7 // %s // X %.1f Y %.1f Z %.1f // STA %.0f // CAM %.2f/%.2f/%.1f SH %s // ORBIT %s // 1 FOOT 2 BOARD 3 BMX 4 CAR* // AUM %s",
                locomotionLabel(player_.locomotion),
                player_.x,
                player_.y,
                player_.z,
                player_.stamina,
                debugRenderer_.cameraYaw(),
                debugRenderer_.cameraPitch(),
                debugRenderer_.cameraDistance(),
                debugRenderer_.cameraShoulderSide() > 0.0f ? "R" : "L",
                cameraDragging_ ? "ACTIVE" : "READY",
                aumPhase
            );
        }
    }
    SDL_SetWindowTitle(window_, title);
}

void HakuiApp::update(float dt)
{
    if (!paused_) {
        world_.elapsedSeconds += dt;
        updateCameraOrbitInput();

        // Spiral advances as part of the same native client heartbeat, but its
        // internals remain platform-independent.
        spiral_.tick(dt);

        const bool* keys = SDL_GetKeyboardState(nullptr);
        float inputRight =
            static_cast<float>(keys[SDL_SCANCODE_D]) -
            static_cast<float>(keys[SDL_SCANCODE_A]);
        float inputForward =
            static_cast<float>(keys[SDL_SCANCODE_W]) -
            static_cast<float>(keys[SDL_SCANCODE_S]);
        bool sprint = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];

        if (gamepad_) {
            inputRight += gamepadAxis(
                SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFTX)
            );
            inputForward -= gamepadAxis(
                SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_LEFTY)
            );
            sprint = sprint ||
                SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER) > 11000 ||
                SDL_GetGamepadButton(gamepad_, SDL_GAMEPAD_BUTTON_LEFT_STICK);

            const float cameraRight = gamepadAxis(
                SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_RIGHTX)
            );
            const float cameraUp = gamepadAxis(
                SDL_GetGamepadAxis(gamepad_, SDL_GAMEPAD_AXIS_RIGHTY)
            );
            if (cameraRight != 0.0f || cameraUp != 0.0f) {
                debugRenderer_.orbitCamera(
                    cameraRight * 520.0f * dt,
                    cameraUp * 420.0f * dt
                );
            }
        }

        inputRight = std::clamp(inputRight, -1.0f, 1.0f);
        inputForward = std::clamp(inputForward, -1.0f, 1.0f);
        const hakui::math::Vec3 cameraMovement =
            hakui::math::cameraRelativePlanarMovement(
                inputRight,
                inputForward,
                debugRenderer_.movementYaw()
            );

        hakui::MovementInput movementInput;
        movementInput.right = cameraMovement.x;
        movementInput.forward = cameraMovement.z;
        movementInput.sprint = sprint;
        movementInput.jumpPressed = jumpQueued_;
        jumpQueued_ = false;

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
            updateCombat(dt, keys);
        } else {
            movementStep = movement_.update(
                player_,
                movementInput,
                blackRoom_.movementEnvironment(),
                dt
            );
        }

        if (movementStep.jumped) {
            audio_.play(HakuiAudioCue::Jump);
        }
        if (movementStep.landed) {
            audio_.play(HakuiAudioCue::Land);
        }
        if (movementStep.respawned) {
            audio_.play(HakuiAudioCue::VoidRespawn);
            SDL_Log("[HAKUI] BLACK SPACE recovery // respawn %u", player_.voidRespawns);
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
    } else {
        jumpQueued_ = false;
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
        scene.opponentX = combat_.zone().opponentAnchor.x;
        scene.opponentY = combat_.zone().opponentAnchor.y;
        scene.opponentZ = combat_.zone().opponentAnchor.z;
        const hakui::WorldAffordanceVolume* activeZone =
            blackRoom_.affordanceById(combat_.zone().worldAffordanceId);
        scene.opponentYaw = activeZone
            ? activeZone->secondaryAnchor.yaw
            : -1.57079632679f;
        scene.playerHitPulse = playerHitPulse_;
        scene.opponentHitPulse = opponentHitPulse_;
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
