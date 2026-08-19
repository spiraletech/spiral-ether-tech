#include "core/HakuiApp.hpp"

#include <algorithm>
#include <cmath>

bool HakuiApp::boot()
{
    SDL_Log("[HAKUI] booting native client v0.3");

    if (!initPlatform() || !initGPU()) {
        return false;
    }

    previousCounter_ = SDL_GetPerformanceCounter();
    SDL_Log("[HAKUI] WORLD ONLINE");
    SDL_Log("[HAKUI] DATA GRUNGE // ACTIVE");

    if (!avatarSkeleton_.buildDefaultHumanoid()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[HAKUI] avatar skeleton initialization failed");
        return false;
    }

    SDL_Log("[HAKUI] avatar rig ready // %zu bones", avatarSkeleton_.boneCount());
    SDL_Log("[HAKUI] controls // WASD move // SHIFT sprint // 1-4 locomotion modes");
    return true;
}

bool HakuiApp::initPlatform()
{
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_AUDIO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[HAKUI] SDL init failed: %s", SDL_GetError());
        return false;
    }

    window_ = SDL_CreateWindow(
        "PROJECT HAKUI // DATA GRUNGE // v0.3",
        1280,
        720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );

    if (!window_) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[HAKUI] window creation failed: %s", SDL_GetError());
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
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[HAKUI] GPU device creation failed: %s", SDL_GetError());
        return false;
    }

    if (!SDL_ClaimWindowForGPUDevice(gpu_, window_)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[HAKUI] GPU could not claim window: %s", SDL_GetError());
        return false;
    }

    SDL_Log("[HAKUI] renderer backend: %s", SDL_GetGPUDeviceDriver(gpu_));

    if (!debugRenderer_.init(gpu_, window_)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[HAKUI] renderer initialization failed: %s", SDL_GetError());
        return false;
    }

    return true;
}

SDL_AppResult HakuiApp::handleEvent(const SDL_Event& event)
{
    if (event.type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    if (event.type == SDL_EVENT_KEY_DOWN) {
        switch (event.key.key) {
            case SDLK_ESCAPE: return SDL_APP_SUCCESS;
            case SDLK_1: locomotion_.switchTo(LocomotionMode::OnFoot); break;
            case SDLK_2: locomotion_.switchTo(LocomotionMode::Skateboard); break;
            case SDLK_3: locomotion_.switchTo(LocomotionMode::BMX); break;
            case SDLK_4: locomotion_.switchTo(LocomotionMode::Car); break;
            default: break;
        }
    }

    return SDL_APP_CONTINUE;
}

SDL_AppResult HakuiApp::tick()
{
    const Uint64 now = SDL_GetPerformanceCounter();
    const Uint64 frequency = SDL_GetPerformanceFrequency();
    float dt = static_cast<float>(now - previousCounter_) / static_cast<float>(frequency);
    previousCounter_ = now;

    // Avoid simulation explosions after a debugger pause or window stall.
    dt = std::min(dt, 0.1f);

    update(dt);
    return render() ? SDL_APP_CONTINUE : SDL_APP_FAILURE;
}

void HakuiApp::update(float dt)
{
    world_.elapsedSeconds += dt;

    if (player_.locomotion == LocomotionMode::OnFoot) {
        const bool* keys = SDL_GetKeyboardState(nullptr);

        float moveX = 0.0f;
        float moveZ = 0.0f;

        if (keys[SDL_SCANCODE_A]) moveX -= 1.0f;
        if (keys[SDL_SCANCODE_D]) moveX += 1.0f;
        if (keys[SDL_SCANCODE_S]) moveZ -= 1.0f;
        if (keys[SDL_SCANCODE_W]) moveZ += 1.0f;

        const float length = std::sqrt(moveX * moveX + moveZ * moveZ);
        if (length > 0.0001f) {
            moveX /= length;
            moveZ /= length;

            const bool sprint = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
            const float speed = sprint ? 5.75f : 3.25f;

            player_.x += moveX * speed * dt;
            player_.z += moveZ * speed * dt;
            player_.yaw = std::atan2(moveX, moveZ);
        }
    }

    locomotion_.update(dt);

    titleTimer_ += dt;
    if (titleTimer_ >= 0.20f) {
        titleTimer_ = 0.0f;

        char title[160];
        SDL_snprintf(
            title,
            sizeof(title),
            "PROJECT HAKUI // v0.3 // DATA GRUNGE // X %.1f  Z %.1f",
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
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[HAKUI] command buffer failed: %s", SDL_GetError());
        return false;
    }

    SDL_GPUTexture* swapchain = nullptr;
    Uint32 width = 0;
    Uint32 height = 0;

    if (!SDL_WaitAndAcquireGPUSwapchainTexture(commands, window_, &swapchain, &width, &height)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[HAKUI] swapchain acquire failed: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(commands);
        return false;
    }

    if (!swapchain) {
        SDL_CancelGPUCommandBuffer(commands);
        return true;
    }

    if (!debugRenderer_.render(commands, swapchain, width, height, player_)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[HAKUI] world render failed: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(commands);
        return false;
    }

    return SDL_SubmitGPUCommandBuffer(commands);
}

void HakuiApp::shutdown()
{
    SDL_Log("[HAKUI] shutting down");

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
