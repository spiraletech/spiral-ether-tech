#include "render/DebugWorldRenderer.hpp"

#include <algorithm>
#include <cmath>
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

float smoothToward(float current, float target, float response, float deltaSeconds)
{
    const float blend = 1.0f - std::exp(-response * std::max(deltaSeconds, 0.0f));
    return current + (target - current) * blend;
}

} // namespace

DebugWorldRenderer::~DebugWorldRenderer()
{
    shutdown();
}

void DebugWorldRenderer::updateCamera(float deltaSeconds, const PlayerState& player)
{
    if (!cameraInitialized_) {
        cameraTargetX_ = player.x;
        cameraTargetY_ = player.y + 1.25f;
        cameraTargetZ_ = player.z + 0.20f;
        cameraInitialized_ = true;
    }

    cameraTargetX_ = smoothToward(cameraTargetX_, player.x, 8.0f, deltaSeconds);
    cameraTargetY_ = smoothToward(cameraTargetY_, player.y + 1.25f, 8.0f, deltaSeconds);
    cameraTargetZ_ = smoothToward(cameraTargetZ_, player.z + 0.20f, 8.0f, deltaSeconds);
    cameraYaw_ = smoothToward(cameraYaw_, targetCameraYaw_, 14.0f, deltaSeconds);
    cameraPitch_ = smoothToward(cameraPitch_, targetCameraPitch_, 14.0f, deltaSeconds);
    cameraDistance_ = smoothToward(
        cameraDistance_,
        targetCameraDistance_,
        12.0f,
        deltaSeconds
    );
}

void DebugWorldRenderer::orbitCamera(float horizontalPixels, float verticalPixels)
{
    constexpr float sensitivity = 0.0065f;
    targetCameraYaw_ -= horizontalPixels * sensitivity;
    targetCameraPitch_ = std::clamp(
        targetCameraPitch_ - verticalPixels * sensitivity,
        0.12f,
        1.18f
    );
}

void DebugWorldRenderer::zoomCamera(float wheelSteps)
{
    targetCameraDistance_ = std::clamp(
        targetCameraDistance_ - wheelSteps * 0.85f,
        3.5f,
        16.0f
    );
}

void DebugWorldRenderer::resetCamera()
{
    targetCameraYaw_ = 2.40f;
    targetCameraPitch_ = 0.48f;
    targetCameraDistance_ = 9.5f;
}

float DebugWorldRenderer::movementYaw() const noexcept
{
    return targetCameraYaw_;
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

    SDL_Log("[HAKUI] v0.5 procedural 3D renderer online");
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

    const float horizontalDistance = cameraDistance_ * std::cos(cameraPitch_);
    const Vec3 cameraTarget{cameraTargetX_, cameraTargetY_, cameraTargetZ_};
    const Vec3 cameraEye{
        cameraTarget.x + std::sin(cameraYaw_) * horizontalDistance,
        cameraTarget.y + std::sin(cameraPitch_) * cameraDistance_,
        cameraTarget.z + std::cos(cameraYaw_) * horizontalDistance
    };

    const Mat4 view = lookAtLH(cameraEye, cameraTarget, {0.0f, 1.0f, 0.0f});
    const Mat4 projection = perspectiveLH(
        50.0f * (kPi / 180.0f),
        static_cast<float>(width) / static_cast<float>(height),
        0.1f,
        100.0f
    );
    const Mat4 viewProjection = multiply(projection, view);

    auto drawModel = [&](const Mat4& model) {
        const Mat4 mvp = multiply(viewProjection, model);
        SDL_PushGPUVertexUniformData(commands, 0, mvp.m, sizeof(mvp.m));
        SDL_DrawGPUPrimitives(pass, 36, 1, 0, 0);
    };

    auto drawBox = [&](const Vec3& position, const Vec3& dimensions) {
        drawModel(multiply(translation(position), scale(dimensions)));
    };

    // First Hakui world surface.
    drawBox({0.0f, -0.08f, 0.0f}, {30.0f, 0.16f, 30.0f});

    // A readable world grid makes speed, direction, and camera motion visible.
    for (int offset = -7; offset <= 7; ++offset) {
        const float position = static_cast<float>(offset) * 2.0f;
        drawBox({position, 0.015f, 0.0f}, {0.025f, 0.03f, 30.0f});
        drawBox({0.0f, 0.015f, position}, {30.0f, 0.03f, 0.025f});
    }

    // Simple skyline markers give the orbit camera useful parallax targets.
    drawBox({-7.0f, 1.0f, 7.0f}, {1.4f, 2.0f, 1.4f});
    drawBox({7.0f, 1.8f, 7.0f}, {1.2f, 3.6f, 1.2f});
    drawBox({-7.0f, 2.6f, -7.0f}, {1.1f, 5.2f, 1.1f});
    drawBox({7.0f, 1.35f, -7.0f}, {1.7f, 2.7f, 1.7f});

    // HAKUI PROCEDURAL HUMANOID v0.5. The canonical 23-bone schema remains
    // data-only for now, but the visible proxy finally has a readable gait.
    const float gait = std::sin(player.gaitPhase);
    const float counterGait = std::sin(player.gaitPhase + kPi);
    const float stride = 0.72f * player.movementBlend;
    const float armStride = 0.82f * player.movementBlend;
    const float idleBreath = 0.012f * std::sin(player.gaitPhase * 0.35f);
    const float bodyBob =
        0.045f * std::abs(std::sin(player.gaitPhase)) * player.movementBlend;
    const float bodySway = 0.035f * gait * player.movementBlend;

    const Mat4 avatarRoot = multiply(
        translation({player.x, player.y + bodyBob, player.z}),
        rotationY(player.yaw)
    );

    auto localBox = [&](const Vec3& position, const Vec3& dimensions) {
        drawModel(multiply(
            avatarRoot,
            multiply(translation(position), scale(dimensions))
        ));
    };

    auto hingedBox = [&](const Vec3& joint,
                         float angle,
                         const Vec3& centerOffset,
                         const Vec3& dimensions) {
        const Mat4 hinge = multiply(
            avatarRoot,
            multiply(translation(joint), rotationX(angle))
        );
        drawModel(multiply(
            hinge,
            multiply(translation(centerOffset), scale(dimensions))
        ));
    };

    auto leg = [&](float side, float angle) {
        const Vec3 hip{side * 0.23f, 1.17f, 0.0f};
        hingedBox(hip, angle, {0.0f, -0.55f, 0.0f}, {0.30f, 1.10f, 0.34f});
        hingedBox(hip, angle, {0.0f, -1.09f, 0.15f}, {0.32f, 0.16f, 0.58f});
    };

    auto arm = [&](float side, float angle) {
        hingedBox(
            {side * 0.60f, 2.12f, 0.0f},
            angle,
            {0.0f, -0.50f, 0.0f},
            {0.24f, 1.00f, 0.28f}
        );
    };

    leg(-1.0f, gait * stride);
    leg(1.0f, counterGait * stride);
    localBox({0.0f, 1.20f, 0.0f}, {0.74f, 0.30f, 0.44f});

    const Mat4 torso = multiply(
        avatarRoot,
        multiply(
            translation({0.0f, 1.72f + idleBreath, 0.0f}),
            multiply(rotationZ(bodySway), scale({0.92f, 0.94f, 0.48f}))
        )
    );
    drawModel(torso);

    arm(-1.0f, counterGait * armStride);
    arm(1.0f, gait * armStride);
    localBox({0.0f, 2.28f + idleBreath, 0.0f}, {0.22f, 0.18f, 0.22f});
    localBox({0.0f, 2.60f + idleBreath, 0.0f}, {0.56f, 0.58f, 0.52f});

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
