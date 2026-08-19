#include "render/DebugWorldRenderer.hpp"

#include <cstring>

#include "render/Math3D.hpp"

// Temporary bootstrap shaders from SDL's own GPU test suite.
// They are fetched with SDL 3.4.14 and covered by SDL's zlib license.
#include "testgpu/cube.frag.dxil.h"
#include "testgpu/cube.frag.msl.h"
#include "testgpu/cube.frag.spv.h"
#include "testgpu/cube.vert.dxil.h"
#include "testgpu/cube.vert.msl.h"
#include "testgpu/cube.vert.spv.h"

namespace {

struct Vertex {
    float x;
    float y;
    float z;
    float r;
    float g;
    float b;
};

constexpr Vertex kCubeVertices[] = {
    // Front
    {-0.5f,  0.5f, -0.5f, 0.76f, 0.76f, 0.78f},
    { 0.5f, -0.5f, -0.5f, 0.76f, 0.76f, 0.78f},
    {-0.5f, -0.5f, -0.5f, 0.76f, 0.76f, 0.78f},
    {-0.5f,  0.5f, -0.5f, 0.76f, 0.76f, 0.78f},
    { 0.5f,  0.5f, -0.5f, 0.76f, 0.76f, 0.78f},
    { 0.5f, -0.5f, -0.5f, 0.76f, 0.76f, 0.78f},

    // Left
    {-0.5f,  0.5f,  0.5f, 0.58f, 0.58f, 0.61f},
    {-0.5f, -0.5f, -0.5f, 0.58f, 0.58f, 0.61f},
    {-0.5f, -0.5f,  0.5f, 0.58f, 0.58f, 0.61f},
    {-0.5f,  0.5f,  0.5f, 0.58f, 0.58f, 0.61f},
    {-0.5f,  0.5f, -0.5f, 0.58f, 0.58f, 0.61f},
    {-0.5f, -0.5f, -0.5f, 0.58f, 0.58f, 0.61f},

    // Top
    {-0.5f, 0.5f,  0.5f, 0.88f, 0.88f, 0.90f},
    { 0.5f, 0.5f, -0.5f, 0.88f, 0.88f, 0.90f},
    {-0.5f, 0.5f, -0.5f, 0.88f, 0.88f, 0.90f},
    {-0.5f, 0.5f,  0.5f, 0.88f, 0.88f, 0.90f},
    { 0.5f, 0.5f,  0.5f, 0.88f, 0.88f, 0.90f},
    { 0.5f, 0.5f, -0.5f, 0.88f, 0.88f, 0.90f},

    // Right
    {0.5f,  0.5f, -0.5f, 0.48f, 0.48f, 0.51f},
    {0.5f, -0.5f,  0.5f, 0.48f, 0.48f, 0.51f},
    {0.5f, -0.5f, -0.5f, 0.48f, 0.48f, 0.51f},
    {0.5f,  0.5f, -0.5f, 0.48f, 0.48f, 0.51f},
    {0.5f,  0.5f,  0.5f, 0.48f, 0.48f, 0.51f},
    {0.5f, -0.5f,  0.5f, 0.48f, 0.48f, 0.51f},

    // Back
    { 0.5f,  0.5f, 0.5f, 0.40f, 0.40f, 0.43f},
    {-0.5f, -0.5f, 0.5f, 0.40f, 0.40f, 0.43f},
    { 0.5f, -0.5f, 0.5f, 0.40f, 0.40f, 0.43f},
    { 0.5f,  0.5f, 0.5f, 0.40f, 0.40f, 0.43f},
    {-0.5f,  0.5f, 0.5f, 0.40f, 0.40f, 0.43f},
    {-0.5f, -0.5f, 0.5f, 0.40f, 0.40f, 0.43f},

    // Bottom
    {-0.5f, -0.5f, -0.5f, 0.31f, 0.31f, 0.34f},
    { 0.5f, -0.5f,  0.5f, 0.31f, 0.31f, 0.34f},
    {-0.5f, -0.5f,  0.5f, 0.31f, 0.31f, 0.34f},
    {-0.5f, -0.5f, -0.5f, 0.31f, 0.31f, 0.34f},
    { 0.5f, -0.5f, -0.5f, 0.31f, 0.31f, 0.34f},
    { 0.5f, -0.5f,  0.5f, 0.31f, 0.31f, 0.34f}
};

constexpr float kPi = 3.14159265358979323846f;

} // namespace

DebugWorldRenderer::~DebugWorldRenderer()
{
    shutdown();
}

bool DebugWorldRenderer::init(SDL_GPUDevice* device, SDL_Window* window)
{
    device_ = device;
    window_ = window;

    if (!device_ || !window_) {
        return false;
    }

    if (!createCubeBuffer()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[HAKUI] cube vertex buffer creation failed: %s", SDL_GetError());
        return false;
    }

    if (!createPipeline()) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[HAKUI] debug graphics pipeline creation failed: %s", SDL_GetError());
        return false;
    }

    SDL_Log("[HAKUI] v0.3 debug 3D renderer online");
    return true;
}

SDL_GPUShader* DebugWorldRenderer::loadCubeShader(bool vertexShader)
{
    SDL_GPUShaderCreateInfo createInfo{};
    createInfo.stage = vertexShader ? SDL_GPU_SHADERSTAGE_VERTEX : SDL_GPU_SHADERSTAGE_FRAGMENT;
    createInfo.num_uniform_buffers = vertexShader ? 1 : 0;

    const SDL_GPUShaderFormat formats = SDL_GetGPUShaderFormats(device_);

    if (formats & SDL_GPU_SHADERFORMAT_DXIL) {
        createInfo.format = SDL_GPU_SHADERFORMAT_DXIL;
        createInfo.code = vertexShader ? cube_vert_dxil : cube_frag_dxil;
        createInfo.code_size = vertexShader ? cube_vert_dxil_len : cube_frag_dxil_len;
    } else if (formats & SDL_GPU_SHADERFORMAT_MSL) {
        createInfo.format = SDL_GPU_SHADERFORMAT_MSL;
        createInfo.code = vertexShader ? cube_vert_msl : cube_frag_msl;
        createInfo.code_size = vertexShader ? cube_vert_msl_len : cube_frag_msl_len;
    } else if (formats & SDL_GPU_SHADERFORMAT_SPIRV) {
        createInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
        createInfo.code = vertexShader ? cube_vert_spv : cube_frag_spv;
        createInfo.code_size = vertexShader ? cube_vert_spv_len : cube_frag_spv_len;
    } else {
        SDL_SetError("Hakui debug renderer has no compatible cube shader format");
        return nullptr;
    }

    return SDL_CreateGPUShader(device_, &createInfo);
}

bool DebugWorldRenderer::createCubeBuffer()
{
    SDL_GPUBufferCreateInfo bufferInfo{};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufferInfo.size = sizeof(kCubeVertices);

    cubeVertexBuffer_ = SDL_CreateGPUBuffer(device_, &bufferInfo);
    if (!cubeVertexBuffer_) {
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transferInfo{};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = sizeof(kCubeVertices);

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device_, &transferInfo);
    if (!transfer) {
        return false;
    }

    void* mapped = SDL_MapGPUTransferBuffer(device_, transfer, false);
    if (!mapped) {
        SDL_ReleaseGPUTransferBuffer(device_, transfer);
        return false;
    }

    std::memcpy(mapped, kCubeVertices, sizeof(kCubeVertices));
    SDL_UnmapGPUTransferBuffer(device_, transfer);

    SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(device_);
    if (!commands) {
        SDL_ReleaseGPUTransferBuffer(device_, transfer);
        return false;
    }

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commands);
    if (!copyPass) {
        SDL_CancelGPUCommandBuffer(commands);
        SDL_ReleaseGPUTransferBuffer(device_, transfer);
        return false;
    }

    SDL_GPUTransferBufferLocation source{};
    source.transfer_buffer = transfer;
    source.offset = 0;

    SDL_GPUBufferRegion destination{};
    destination.buffer = cubeVertexBuffer_;
    destination.offset = 0;
    destination.size = sizeof(kCubeVertices);

    SDL_UploadToGPUBuffer(copyPass, &source, &destination, false);
    SDL_EndGPUCopyPass(copyPass);

    const bool submitted = SDL_SubmitGPUCommandBuffer(commands);
    SDL_ReleaseGPUTransferBuffer(device_, transfer);
    return submitted;
}

bool DebugWorldRenderer::createPipeline()
{
    SDL_GPUShader* vertexShader = loadCubeShader(true);
    if (!vertexShader) {
        return false;
    }

    SDL_GPUShader* fragmentShader = loadCubeShader(false);
    if (!fragmentShader) {
        SDL_ReleaseGPUShader(device_, vertexShader);
        return false;
    }

    SDL_GPUColorTargetDescription colorTarget{};
    colorTarget.format = SDL_GetGPUSwapchainTextureFormat(device_, window_);

    SDL_GPUVertexBufferDescription vertexBuffer{};
    vertexBuffer.slot = 0;
    vertexBuffer.pitch = sizeof(Vertex);
    vertexBuffer.input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBuffer.instance_step_rate = 0;

    SDL_GPUVertexAttribute attributes[2]{};
    attributes[0].location = 0;
    attributes[0].buffer_slot = 0;
    attributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attributes[0].offset = 0;

    attributes[1].location = 1;
    attributes[1].buffer_slot = 0;
    attributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    attributes[1].offset = sizeof(float) * 3;

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo{};
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = &vertexBuffer;
    pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
    pipelineInfo.vertex_input_state.vertex_attributes = attributes;

    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = &colorTarget;
    pipelineInfo.target_info.has_depth_stencil_target = true;
    pipelineInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;

    pipelineInfo.depth_stencil_state.enable_depth_test = true;
    pipelineInfo.depth_stencil_state.enable_depth_write = true;
    pipelineInfo.depth_stencil_state.compare_op = SDL_GPU_COMPAREOP_LESS_OR_EQUAL;

    pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_NONE;
    pipelineInfo.rasterizer_state.enable_depth_clip = true;
    pipelineInfo.multisample_state.sample_count = SDL_GPU_SAMPLECOUNT_1;

    pipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &pipelineInfo);

    SDL_ReleaseGPUShader(device_, vertexShader);
    SDL_ReleaseGPUShader(device_, fragmentShader);

    return pipeline_ != nullptr;
}

bool DebugWorldRenderer::ensureDepthTexture(Uint32 width, Uint32 height)
{
    if (width == 0 || height == 0) {
        return false;
    }

    if (depthTexture_ && depthWidth_ == width && depthHeight_ == height) {
        return true;
    }

    if (depthTexture_) {
        SDL_ReleaseGPUTexture(device_, depthTexture_);
        depthTexture_ = nullptr;
    }

    SDL_GPUTextureCreateInfo depthInfo{};
    depthInfo.type = SDL_GPU_TEXTURETYPE_2D;
    depthInfo.format = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    depthInfo.width = width;
    depthInfo.height = height;
    depthInfo.layer_count_or_depth = 1;
    depthInfo.num_levels = 1;
    depthInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
    depthInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;

    depthTexture_ = SDL_CreateGPUTexture(device_, &depthInfo);
    if (!depthTexture_) {
        return false;
    }

    depthWidth_ = width;
    depthHeight_ = height;
    return true;
}

bool DebugWorldRenderer::render(
    SDL_GPUCommandBuffer* commands,
    SDL_GPUTexture* swapchain,
    Uint32 width,
    Uint32 height,
    const PlayerState& player)
{
    using namespace hakui::math;

    if (!commands || !swapchain) {
        return true;
    }

    if (!ensureDepthTexture(width, height)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION, "[HAKUI] depth texture creation failed: %s", SDL_GetError());
        return false;
    }

    SDL_GPUColorTargetInfo colorTarget{};
    colorTarget.texture = swapchain;
    colorTarget.clear_color = SDL_FColor{0.018f, 0.018f, 0.022f, 1.0f};
    colorTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTarget.store_op = SDL_GPU_STOREOP_STORE;

    SDL_GPUDepthStencilTargetInfo depthTarget{};
    depthTarget.texture = depthTexture_;
    depthTarget.clear_depth = 1.0f;
    depthTarget.load_op = SDL_GPU_LOADOP_CLEAR;
    depthTarget.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthTarget.stencil_load_op = SDL_GPU_LOADOP_DONT_CARE;
    depthTarget.stencil_store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthTarget.cycle = true;

    SDL_GPURenderPass* pass = SDL_BeginGPURenderPass(commands, &colorTarget, 1, &depthTarget);
    if (!pass) {
        return false;
    }

    SDL_BindGPUGraphicsPipeline(pass, pipeline_);

    SDL_GPUBufferBinding vertexBinding{};
    vertexBinding.buffer = cubeVertexBuffer_;
    vertexBinding.offset = 0;
    SDL_BindGPUVertexBuffers(pass, 0, &vertexBinding, 1);

    const Vec3 playerPosition{player.x, player.y, player.z};
    const Vec3 cameraEye{player.x + 6.5f, player.y + 5.8f, player.z - 7.0f};
    const Vec3 cameraTarget{player.x, player.y + 1.25f, player.z + 0.45f};

    const Mat4 view = lookAtLH(cameraEye, cameraTarget, {0.0f, 1.0f, 0.0f});
    const Mat4 projection = perspectiveLH(
        50.0f * (kPi / 180.0f),
        static_cast<float>(width) / static_cast<float>(height),
        0.1f,
        100.0f
    );
    const Mat4 viewProjection = multiply(projection, view);

    auto drawBox = [&](const Vec3& position, const Vec3& dimensions) {
        const Mat4 model = multiply(translation(position), scale(dimensions));
        const Mat4 mvp = multiply(viewProjection, model);
        SDL_PushGPUVertexUniformData(commands, 0, mvp.m, sizeof(mvp.m));
        SDL_DrawGPUPrimitives(pass, 36, 1, 0, 0);
    };

    // First Hakui world surface.
    drawBox({0.0f, -0.08f, 0.0f}, {30.0f, 0.16f, 30.0f});

    // HAKUI HUMANOID DEBUG PROXY v0.1
    // This is intentionally primitive geometry. It lets us validate the camera,
    // world scale, locomotion, and rig proportions before GPU skinning lands.
    drawBox({playerPosition.x - 0.23f, 0.08f, playerPosition.z + 0.12f}, {0.32f, 0.16f, 0.58f});
    drawBox({playerPosition.x + 0.23f, 0.08f, playerPosition.z + 0.12f}, {0.32f, 0.16f, 0.58f});

    drawBox({playerPosition.x - 0.23f, 0.62f, playerPosition.z}, {0.30f, 1.10f, 0.34f});
    drawBox({playerPosition.x + 0.23f, 0.62f, playerPosition.z}, {0.30f, 1.10f, 0.34f});

    drawBox({playerPosition.x, 1.20f, playerPosition.z}, {0.74f, 0.30f, 0.44f});
    drawBox({playerPosition.x, 1.72f, playerPosition.z}, {0.92f, 0.94f, 0.48f});

    drawBox({playerPosition.x - 0.60f, 1.70f, playerPosition.z}, {0.24f, 1.00f, 0.28f});
    drawBox({playerPosition.x + 0.60f, 1.70f, playerPosition.z}, {0.24f, 1.00f, 0.28f});

    drawBox({playerPosition.x, 2.28f, playerPosition.z}, {0.22f, 0.18f, 0.22f});
    drawBox({playerPosition.x, 2.60f, playerPosition.z}, {0.56f, 0.58f, 0.52f});

    SDL_EndGPURenderPass(pass);
    return true;
}

void DebugWorldRenderer::shutdown()
{
    if (!device_) {
        return;
    }

    if (depthTexture_) {
        SDL_ReleaseGPUTexture(device_, depthTexture_);
        depthTexture_ = nullptr;
    }

    if (pipeline_) {
        SDL_ReleaseGPUGraphicsPipeline(device_, pipeline_);
        pipeline_ = nullptr;
    }

    if (cubeVertexBuffer_) {
        SDL_ReleaseGPUBuffer(device_, cubeVertexBuffer_);
        cubeVertexBuffer_ = nullptr;
    }

    depthWidth_ = 0;
    depthHeight_ = 0;
    device_ = nullptr;
    window_ = nullptr;
}
