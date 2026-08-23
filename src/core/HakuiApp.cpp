#include "core/HakuiApp.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

#include "render/Math3D.hpp"

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

} // namespace

bool HakuiApp::boot()
{
    SDL_Log("[HAKUI] booting native client v0.8-dev // CONTROL NERVOUS SYSTEM");

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
    SDL_Log("[HAKUI] v0.8 input // hardware -> intent -> discipline online");
    SDL_Log("[HAKUI] procedural locomotion // idle + walk + sprint + jump + seated online");
    SDL_Log("[HAKUI] BLACK ROOM // neon lounge + couch + fusion table + open void");
    SDL_Log("[HAKUI] controls // WASD move // SPACE jump // E interact/stand // SHIFT sprint");
    SDL_Log("[HAKUI] camera // RMB orbit // WHEEL zoom // TAB shoulder // R reset");
    SDL_Log("[HAKUI] menu // ESC pause // [ ] look sensitivity // - + audio // F10 quit");
    SDL_Log("[HAKUI] ride interface // controller native // keyboard fallback F9 developer-only");
    SDL_Log("[HAKUI] ride verbs // SOUTH hop // LB manual // RB grind // WEST/NORTH trick");
    SDL_Log("[HAKUI] modes // DPAD UP foot // LEFT skateboard // RIGHT BMX");
    SDL_Log("[HAKUI] FUSION TABLE // T dice // G cards // B bet 25 // H hit // J stand // I inspect");
    SDL_Log("[HAKUI] tabletop input is locked until seated // virtual credits only");
    SDL_Log("[HAKUI] combat grammar // semantic primary/secondary // guard // recover");
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
        "SPIRAL OS: HAKUI ENGINE // v0.8-dev // CONTROL NERVOUS SYSTEM",
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
    bootSignal.payload = "hakui-v0.8-dev";

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
        {"client.version", std::string("0.8-dev")},
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

void HakuiApp::handleCasinoAction(hakui::input::Action action)
{
    if (player_.activity != PlayerActivity::CasinoSeated || !terminal_) {
        SDL_Log("[TABLETOP] control anchored to table // press E at the chair");
        return;
    }

    using hakui::input::Action;
    switch (action) {
        case Action::TerminalUse:
            interactWithTerminal(hakui::InteractionVerb::Use);
            if (terminal_->lastDiceReward() > 0) {
                SDL_Log(
                    "[TABLETOP] doubles // earned %lld virtual credits",
                    static_cast<long long>(terminal_->lastDiceReward())
                );
            }
            break;

        case Action::CardSuite:
            interactWithTerminal(hakui::InteractionVerb::Play);
            break;

        case Action::Inspect:
            interactWithTerminal(hakui::InteractionVerb::Inspect);
            break;

        case Action::Bet:
            if (terminal_->beginCardRound(25)) {
                SDL_Log("[TABLETOP] round started // virtual wager 25");
                audio_.play(HakuiAudioCue::Casino);
            } else {
                SDL_Log("[TABLETOP] press G for cards, then B to wager 25");
            }
            break;

        case Action::CardHit:
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

        case Action::CardStand:
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
        SDL_Log("[HAKUI] controller offline");
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
    return render() ? SDL_APP_CONTINUE : SDL_APP_FAILURE;
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
        return InputResolver::prompt(action, inputFrame_.activeDevice);
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

    char title[768];
    if (inputStatusTimer_ > 0.0f && !inputStatus_.empty()) {
        SDL_snprintf(
            title,
            sizeof(title),
            "HAKUI v0.8 // %s // INPUT %.*s",
            inputStatus_.c_str(),
            static_cast<int>(device.size()), device.data()
        );
    } else if (paused_) {
        SDL_snprintf(
            title,
            sizeof(title),
            "HAKUI v0.8 // PAUSED // %.*s RESUME // LOOK %.4f // AUDIO %.0f%% // INPUT %.*s // MOUSE RELEASED",
            static_cast<int>(pause.size()), pause.data(),
            debugRenderer_.lookSensitivity(),
            audio_.volume() * 100.0f,
            static_cast<int>(device.size()), device.data()
        );
    } else if (combat_.active()) {
        SDL_snprintf(
            title,
            sizeof(title),
            "HAKUI v0.8 // SPAR %s // HP %.0f STA %.0f BAL %.0f // TARGET HP %.0f BAL %.0f // MOVE FOOTWORK // %.*s PRIMARY // %.*s SECONDARY // %.*s GUARD // %.*s RECOVER // %.*s LEAVE // %.*s",
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
        const auto terminalUse = prompt(Action::TerminalUse);
        const auto cardSuite = prompt(Action::CardSuite);
        const auto bet = prompt(Action::Bet);
        const auto hit = prompt(Action::CardHit);
        const auto stand = prompt(Action::CardStand);
        const int handValue = hakui::games::BlackjackTable::handValue(
            terminal_->cardTable().playerHand()
        );
        SDL_snprintf(
            title,
            sizeof(title),
            "HAKUI v0.8 // FUSION TABLE // CREDITS %lld // HAND %d // DICE %d // %.*s ROLL // %.*s CARDS // %.*s BET // %.*s HIT // %.*s STAND // %.*s LEAVE // %.*s",
            static_cast<long long>(terminal_->virtualCredits()),
            handValue,
            terminal_->lastDiceResult().total,
            static_cast<int>(terminalUse.size()), terminalUse.data(),
            static_cast<int>(cardSuite.size()), cardSuite.data(),
            static_cast<int>(bet.size()), bet.data(),
            static_cast<int>(hit.size()), hit.data(),
            static_cast<int>(stand.size()), stand.data(),
            static_cast<int>(interact.size()), interact.data(),
            static_cast<int>(device.size()), device.data()
        );
    } else if (player_.activity == PlayerActivity::CouchSeated) {
        SDL_snprintf(
            title,
            sizeof(title),
            "HAKUI v0.8 // VOID COUCH // AUM %s // %.*s STAND // %.*s ORBIT // %.*s PAUSE // %.*s",
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
            "HAKUI v0.8 // BLACK SPACE FALL // DEPTH %.1f // RECOVERY ARMED // RESPAWNS %u // %.*s",
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
            "HAKUI v0.8 // %s // %.*s // %.*s // SPD %.1f BAL %.0f // LAND %.*s // COMBO %s // %.*s HOP // %.*s BALANCE // %.*s GRIND // %.*s/%.*s TRICK // %.*s DISMOUNT // %.*s",
            locomotionLabel(player_.locomotion),
            static_cast<int>(phase.size()), phase.data(),
            static_cast<int>(trick.size()), trick.data(),
            ride.speed,
            ride.balance,
            static_cast<int>(landing.size()), landing.data(),
            ride.comboCount > 0 ? combo : "READY",
            static_cast<int>(jump.size()), jump.data(),
            static_cast<int>(prompt(Action::Balance).size()),
            prompt(Action::Balance).data(),
            static_cast<int>(prompt(Action::Grind).size()),
            prompt(Action::Grind).data(),
            static_cast<int>(primary.size()), primary.data(),
            static_cast<int>(secondary.size()), secondary.data(),
            static_cast<int>(cancel.size()), cancel.data(),
            static_cast<int>(device.size()), device.data()
        );
    } else {
        const hakui::RoomInteractionFocus focus = blackRoom_.nearestInteraction(player_);
        if (focus) {
            SDL_snprintf(
                title,
                sizeof(title),
                "HAKUI v0.8 // %.*s // %.*s USE // %.*s ORBIT // MODE %s // STAMINA %.0f // AUM %s // %.*s",
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
                "HAKUI v0.8 // SPARRING DATUM // %.*s ENTER // DUMMY VISIBLE // %.*s/%.*s ATTACK // %.*s GUARD // %.*s RECOVER // %.*s",
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
                "HAKUI v0.8 // %s // X %.1f Y %.1f Z %.1f // STA %.0f // CAM %.2f/%.2f/%.1f SH %s // ORBIT %s // INTENT DEVICE %.*s // AUM %s",
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

        // Spiral advances as part of the same native client heartbeat, but its
        // internals remain platform-independent.
        spiral_.tick(dt);

        if (player_.activity == PlayerActivity::Roaming &&
            inputFrame_.action(Action::CameraShoulder).pressed) {
            debugRenderer_.toggleShoulder();
        }
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
                handlePrimaryInteraction();
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

        constexpr Action tableActions[] = {
            Action::TerminalUse,
            Action::CardSuite,
            Action::Bet,
            Action::CardHit,
            Action::CardStand,
            Action::Inspect
        };
        if (player_.activity == PlayerActivity::CasinoSeated) {
            for (const Action action : tableActions) {
                if (inputFrame_.action(action).pressed) {
                    handleCasinoAction(action);
                }
            }
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
        movementInput.right = cameraMovement.x;
        movementInput.forward = cameraMovement.z;
        movementInput.sprint = embodimentIntent.accelerate > 0.20f;
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
            rideInput.hopPressed =
                embodimentIntent.traversal == hakui::input::TraversalIntent::Ollie ||
                embodimentIntent.traversal == hakui::input::TraversalIntent::BunnyHop;
            rideInput.manualHeld = embodimentIntent.balance;
            rideInput.grindHeld = embodimentIntent.grind;
            rideInput.flipLeftPressed = embodimentIntent.primary;
            rideInput.flipRightPressed = embodimentIntent.secondary;
            const hakui::RideableFrame rideFrame = rideable_.update(
                player_,
                rideInput,
                blackRoom_.movementEnvironment(),
                blackRoom_.affordances(),
                dt
            );
            movementStep = rideFrame.movement;
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
            }
            if (rideFrame.bailed) {
                audio_.play(HakuiAudioCue::CombatHit);
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
