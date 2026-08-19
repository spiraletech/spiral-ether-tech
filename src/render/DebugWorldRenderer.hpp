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
};
