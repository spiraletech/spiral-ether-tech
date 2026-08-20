#include "core/HakuiApp.hpp"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <utility>

#include "render/Math3D.hpp"

bool HakuiApp::boot()
{
    SDL_Log("[HAKUI] booting native client v0.5-dev");

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
    previousCounter_ = SDL_GetPerformanceCounter();

    SDL_Log("[HAKUI] WORLD ONLINE");
    SDL_Log("[HAKUI] DATA GRUNGE // ACTIVE");
    SDL_Log("[HAKUI] SPIRAL CORE // ONLINE");
    SDL_Log("[HAKUI] avatar skeleton // %zu bones loaded", avatarSkeleton_.boneCount());
    SDL_Log("[HAKUI] procedural locomotion // idle + walk + sprint online");
    SDL_Log("[HAKUI] controls // WASD move // SHIFT sprint // RMB orbit // WHEEL zoom // R camera reset");
    SDL_Log("[HAKUI] modes // 1-4 locomotion scaffolds");
    SDL_Log("[HAKUI] terminal // T use/dice // G cards // B bet 25 // H hit // J stand // I inspect");
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
        "PROJECT HAKUI // DATA GRUNGE // v0.5-dev",
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
    bootSignal.payload = "hakui-v0.5-dev";

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
        {"client.version", std::string("0.5-dev")},
        {"avatar.rig.bones", static_cast<std::int64_t>(avatarSkeleton_.boneCount())},
        {"player.locomotion", std::string("on_foot")}
    };
    spiral_.dispatch(std::move(initialState));
}

void HakuiApp::switchLocomotion(LocomotionMode mode, std::string_view label)
{
    locomotion_.switchTo(mode);

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
    if (!terminal_) {
        return;
    }

    hakui::InteractionRequest request;
    request.actor = 1;
    request.target = terminal_->interactionId();
    request.verb = verb;

    const hakui::InteractionResult result = interactions_.interact(request);
    if (result.handled) {
        SDL_Log("[TERMINAL] %s", result.output.c_str());
    } else {
        SDL_Log("[TERMINAL] action unavailable");
    }
}

SDL_AppResult HakuiApp::handleEvent(const SDL_Event& event)
{
    if (event.type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
        event.button.button == SDL_BUTTON_RIGHT) {
        cameraDragging_ = true;
        SDL_SetWindowRelativeMouseMode(window_, true);
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
        event.button.button == SDL_BUTTON_RIGHT) {
        cameraDragging_ = false;
        SDL_SetWindowRelativeMouseMode(window_, false);
    }

    if (event.type == SDL_EVENT_WINDOW_FOCUS_LOST && cameraDragging_) {
        cameraDragging_ = false;
        SDL_SetWindowRelativeMouseMode(window_, false);
    }

    if (event.type == SDL_EVENT_MOUSE_MOTION && cameraDragging_) {
        debugRenderer_.orbitCamera(event.motion.xrel, event.motion.yrel);
    }

    if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        const float wheelDirection =
            event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED ? -1.0f : 1.0f;
        debugRenderer_.zoomCamera(event.wheel.y * wheelDirection);
    }

    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
        switch (event.key.key) {
            case SDLK_ESCAPE:
                return SDL_APP_SUCCESS;

            case SDLK_R:
                debugRenderer_.resetCamera();
                break;

            case SDLK_1:
                switchLocomotion(LocomotionMode::OnFoot, "on_foot");
                break;

            case SDLK_2:
                switchLocomotion(LocomotionMode::Skateboard, "skateboard");
                break;

            case SDLK_3:
                switchLocomotion(LocomotionMode::BMX, "bmx");
                break;

            case SDLK_4:
                switchLocomotion(LocomotionMode::Car, "car");
                break;

            case SDLK_T:
                interactWithTerminal(hakui::InteractionVerb::Use);
                break;

            case SDLK_G:
                interactWithTerminal(hakui::InteractionVerb::Play);
                break;

            case SDLK_I:
                interactWithTerminal(hakui::InteractionVerb::Inspect);
                break;

            case SDLK_B:
                if (terminal_ && terminal_->beginCardRound(25)) {
                    SDL_Log("[TABLETOP] round started // virtual wager 25");
                } else {
                    SDL_Log("[TABLETOP] open card suite before starting a round");
                }
                break;

            case SDLK_H:
                if (terminal_ && terminal_->hitCardTable()) {
                    SDL_Log(
                        "[TABLETOP] hit // hand value %d",
                        hakui::games::BlackjackTable::handValue(
                            terminal_->cardTable().playerHand()
                        )
                    );
                }
                break;

            case SDLK_J:
                if (terminal_ && terminal_->standCardTable()) {
                    SDL_Log(
                        "[TABLETOP] stand // virtual credits %lld",
                        static_cast<long long>(terminal_->cardTable().credits())
                    );
                }
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

void HakuiApp::update(float dt)
{
    world_.elapsedSeconds += dt;

    // Spiral advances as part of the same native client heartbeat, but its
    // internals remain platform-independent.
    spiral_.tick(dt);

    const bool* keys = SDL_GetKeyboardState(nullptr);
    hakui::MovementInput movementInput;
    const float inputRight =
        static_cast<float>(keys[SDL_SCANCODE_D]) -
        static_cast<float>(keys[SDL_SCANCODE_A]);
    const float inputForward =
        static_cast<float>(keys[SDL_SCANCODE_W]) -
        static_cast<float>(keys[SDL_SCANCODE_S]);
    const hakui::math::Vec3 cameraMovement = hakui::math::cameraRelativePlanarMovement(
        inputRight,
        inputForward,
        debugRenderer_.movementYaw()
    );
    movementInput.right = cameraMovement.x;
    movementInput.forward = cameraMovement.z;
    movementInput.sprint =
        keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];

    const hakui::MovementStep movementStep = movement_.update(player_, movementInput, dt);

    player_.sprinting = movementStep.sprinting;
    const float targetMovementBlend = movementStep.moved
        ? (movementStep.sprinting ? 1.0f : 0.62f)
        : 0.0f;
    const float blendResponse = 1.0f - std::exp(-12.0f * dt);
    player_.movementBlend +=
        (targetMovementBlend - player_.movementBlend) * blendResponse;
    player_.idlePhase += 1.8f * dt;
    if (movementStep.moved) {
        const float gaitSpeed = movementStep.sprinting ? 11.0f : 7.2f;
        player_.gaitPhase += gaitSpeed * dt;
    }

    debugRenderer_.updateCamera(dt, player_);

    locomotion_.update(dt);

    titleTimer_ += dt;
    if (titleTimer_ >= 0.20f) {
        titleTimer_ = 0.0f;

        const char* aumPhase = "A";
        switch (spiral_.aumField().phase()) {
            case spiral::AUMPhase::A_Emergence: aumPhase = "A"; break;
            case spiral::AUMPhase::U_Sustain:   aumPhase = "U"; break;
            case spiral::AUMPhase::M_Return:    aumPhase = "M"; break;
        }

        char title[224];
        SDL_snprintf(
            title,
            sizeof(title),
            "PROJECT HAKUI // v0.5-dev // AUM %s // EVENTS %zu // STATE R%llu // X %.1f  Z %.1f",
            aumPhase,
            spiral_.monolith().size(),
            static_cast<unsigned long long>(spiral_.stateStore().revision()),
            player_.x,
            player_.z
        );
        SDL_SetWindowTitle(window_, title);
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

    if (!debugRenderer_.render(commands, swapchain, width, height, player_)) {
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

    if (window_ && cameraDragging_) {
        SDL_SetWindowRelativeMouseMode(window_, false);
        cameraDragging_ = false;
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
