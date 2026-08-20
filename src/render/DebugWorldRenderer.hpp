#pragma once

#include <SDL3/SDL.h>

#include "player/PlayerState.hpp"

class DebugWorldRenderer {
public:
    DebugWorldRenderer() = default;
    ~DebugWorldRenderer();

    DebugWorldRenderer(const DebugWorldRenderer&) = delete;
    DebugWorldRenderer& operator=(const DebugWorldRenderer&) = delete;

    bool init(SDL_GPUDevice* device, SDL_Window* window);
    bool render(
        SDL_GPUCommandBuffer* commands,
        SDL_GPUTexture* swapchain,
        Uint32 width,
        Uint32 height,
        const PlayerState& player
    );
    void updateCamera(float deltaSeconds, const PlayerState& player);
    void orbitCamera(float horizontalPixels, float verticalPixels);
    void zoomCamera(float wheelSteps);
    void resetCamera();
    float movementYaw() const noexcept;
    void shutdown();

private:
    bool createCubeBuffer();
    bool createPipeline();
    bool ensureDepthTexture(Uint32 width, Uint32 height);
    SDL_GPUShader* loadCubeShader(bool vertexShader);

private:
    SDL_GPUDevice* device_ = nullptr;
    SDL_Window* window_ = nullptr;
    SDL_GPUBuffer* cubeVertexBuffer_ = nullptr;
    SDL_GPUGraphicsPipeline* pipeline_ = nullptr;
    SDL_GPUTexture* depthTexture_ = nullptr;
    Uint32 depthWidth_ = 0;
    Uint32 depthHeight_ = 0;

    float cameraYaw_ = 2.40f;
    float cameraPitch_ = 0.48f;
    float cameraDistance_ = 9.5f;
    float targetCameraYaw_ = 2.40f;
    float targetCameraPitch_ = 0.48f;
    float targetCameraDistance_ = 9.5f;
    float cameraTargetX_ = 0.0f;
    float cameraTargetY_ = 1.25f;
    float cameraTargetZ_ = 0.0f;
    bool cameraInitialized_ = false;
};
