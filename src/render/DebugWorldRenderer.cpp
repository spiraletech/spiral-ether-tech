#include "render/DebugWorldRenderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

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

enum Palette : Uint32 {
    Shell = 0,
    Midnight,
    Cyan,
    Magenta,
    Amber,
    Void,
    TerminalGreen,
    Danger,
    PaletteCount
};

Uint32 paletteFor(hakui::MaterialRole material)
{
    switch (material) {
        case hakui::MaterialRole::PowderConcrete: return Shell;
        case hakui::MaterialRole::IndustrialDark: return Midnight;
        case hakui::MaterialRole::CrtCyan: return Cyan;
        case hakui::MaterialRole::SignalMagenta: return Magenta;
        case hakui::MaterialRole::SodiumAmber: return Amber;
        case hakui::MaterialRole::VoidBlack: return Void;
        case hakui::MaterialRole::TerminalGreen: return TerminalGreen;
        case hakui::MaterialRole::HazardRed: return Danger;
    }
    return Shell;
}

constexpr std::array<std::array<float, 3>, PaletteCount> kPalettes{{
    {{0.82f, 0.84f, 0.90f}},
    {{0.09f, 0.12f, 0.20f}},
    {{0.06f, 0.92f, 1.00f}},
    {{0.96f, 0.08f, 0.66f}},
    {{1.00f, 0.62f, 0.10f}},
    {{0.012f, 0.014f, 0.024f}},
    {{0.22f, 1.00f, 0.48f}},
    {{1.00f, 0.12f, 0.18f}}
}};

constexpr Uint32 kCubeVertexCount = static_cast<Uint32>(std::size(kCubeVertices));

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
    float desiredTargetX = player.x;
    float desiredTargetY = player.y + 1.25f;
    float desiredTargetZ = player.z + 0.20f;

    if (cameraRole_ == CameraRole::InteractionFrame &&
        interactionFrame_ == InteractionFrame::FusionTable) {
        desiredTargetX = 0.0f;
        desiredTargetY = 1.0f;
        desiredTargetZ = 0.0f;
    } else if (cameraRole_ == CameraRole::CombatFrame ||
               cameraRole_ == CameraRole::TargetFrame ||
               cameraRole_ == CameraRole::DuelFrame) {
        desiredTargetX = (player.x + combatTargetX_) * 0.5f;
        desiredTargetY = (player.y + combatTargetY_) * 0.5f + 0.35f;
        desiredTargetZ = (player.z + combatTargetZ_) * 0.5f;
    } else {
        const float shoulderOffset = 0.58f * shoulderSide_;
        desiredTargetX += -std::cos(targetCameraYaw_) * shoulderOffset;
        desiredTargetZ += std::sin(targetCameraYaw_) * shoulderOffset;
    }

    if (!cameraInitialized_) {
        cameraTargetX_ = desiredTargetX;
        cameraTargetY_ = desiredTargetY;
        cameraTargetZ_ = desiredTargetZ;
        cameraInitialized_ = true;
    }

    cameraTargetX_ = smoothToward(cameraTargetX_, desiredTargetX, 8.0f, deltaSeconds);
    cameraTargetY_ = smoothToward(cameraTargetY_, desiredTargetY, 8.0f, deltaSeconds);
    cameraTargetZ_ = smoothToward(cameraTargetZ_, desiredTargetZ, 8.0f, deltaSeconds);
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
    targetCameraYaw_ -= horizontalPixels * lookSensitivity_;
    targetCameraPitch_ = std::clamp(
        targetCameraPitch_ - verticalPixels * lookSensitivity_,
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

void DebugWorldRenderer::toggleShoulder()
{
    shoulderSide_ *= -1.0f;
}

void DebugWorldRenderer::adjustLookSensitivity(float delta)
{
    lookSensitivity_ = std::clamp(lookSensitivity_ + delta, 0.0025f, 0.016f);
}

float DebugWorldRenderer::lookSensitivity() const noexcept
{
    return lookSensitivity_;
}

void DebugWorldRenderer::setCameraRole(CameraRole role)
{
    if (cameraRole_ == role) {
        return;
    }

    cameraRole_ = role;
    if (role == CameraRole::CombatFrame || role == CameraRole::TargetFrame ||
        role == CameraRole::DuelFrame) {
        targetCameraYaw_ = 3.12f;
        targetCameraPitch_ = 0.38f;
        targetCameraDistance_ = 7.2f;
    } else if (role == CameraRole::InteractionFrame &&
               interactionFrame_ == InteractionFrame::FusionTable) {
        targetCameraYaw_ = 0.0f;
        targetCameraPitch_ = 0.62f;
        targetCameraDistance_ = 5.6f;
    } else if (role == CameraRole::InteractionFrame &&
               interactionFrame_ == InteractionFrame::LoungeCouch) {
        targetCameraYaw_ = 1.85f;
        targetCameraPitch_ = 0.46f;
        targetCameraDistance_ = 5.0f;
    } else {
        targetCameraDistance_ = 8.5f;
    }
}

void DebugWorldRenderer::frameInteraction(InteractionFrame frame)
{
    interactionFrame_ = frame;
    cameraRole_ = CameraRole::GameplayFollow;
    setCameraRole(CameraRole::InteractionFrame);
}

void DebugWorldRenderer::setCombatTarget(float x, float y, float z) noexcept
{
    combatTargetX_ = x;
    combatTargetY_ = y;
    combatTargetZ_ = z;
}

CameraRole DebugWorldRenderer::cameraRole() const noexcept
{
    return cameraRole_;
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

    SDL_Log("[HAKUI] v0.65 DATA GRUNGE renderer online // semantic 8-role material palette");
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
    std::vector<Vertex> vertices;
    vertices.reserve(static_cast<std::size_t>(kCubeVertexCount) * PaletteCount);
    for (const auto& palette : kPalettes) {
        for (const Vertex& source : kCubeVertices) {
            const float shade = source.r;
            vertices.push_back({
                source.x,
                source.y,
                source.z,
                palette[0] * shade,
                palette[1] * shade,
                palette[2] * shade
            });
        }
    }

    const Uint32 vertexBytes = static_cast<Uint32>(vertices.size() * sizeof(Vertex));
    SDL_GPUBufferCreateInfo bufferInfo{};
    bufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    bufferInfo.size = vertexBytes;

    cubeVertexBuffer_ = SDL_CreateGPUBuffer(device_, &bufferInfo);
    if (!cubeVertexBuffer_) {
        return false;
    }

    SDL_GPUTransferBufferCreateInfo transferInfo{};
    transferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    transferInfo.size = vertexBytes;

    SDL_GPUTransferBuffer* transfer = SDL_CreateGPUTransferBuffer(device_, &transferInfo);
    if (!transfer) {
        return false;
    }

    void* mapped = SDL_MapGPUTransferBuffer(device_, transfer, false);
    if (!mapped) {
        SDL_ReleaseGPUTransferBuffer(device_, transfer);
        return false;
    }

    std::memcpy(mapped, vertices.data(), vertexBytes);
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
    destination.size = vertexBytes;

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
    const PlayerState& player,
    const HakuiSceneState& scene,
    std::span<const hakui::WorldPrimitive> worldGeometry)
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
    colorTarget.clear_color = scene.paused
        ? SDL_FColor{0.006f, 0.006f, 0.010f, 1.0f}
        : SDL_FColor{0.002f, 0.003f, 0.008f, 1.0f};
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

    const Vec3 cameraTarget{cameraTargetX_, cameraTargetY_, cameraTargetZ_};
    const float horizontalDistance = cameraDistance_ * std::cos(cameraPitch_);
    Vec3 eyeOffset{
        std::sin(cameraYaw_) * horizontalDistance,
        std::sin(cameraPitch_) * cameraDistance_,
        std::cos(cameraYaw_) * horizontalDistance
    };

    // Camera collision against the Black Room's solid side/back shell. The
    // front remains open so the camera can follow a player falling into void.
    if (cameraTarget.y > -0.5f && cameraTarget.z < 8.0f) {
        float allowed = 1.0f;
        const auto restrictAtPlane = [&](float targetValue,
                                         float offsetValue,
                                         float plane,
                                         bool beyondPositive) {
            const float eyeValue = targetValue + offsetValue;
            const bool crosses = beyondPositive ? eyeValue > plane : eyeValue < plane;
            if (crosses && std::fabs(offsetValue) > 0.0001f) {
                const float fraction = (plane - targetValue) / offsetValue;
                allowed = std::min(allowed, std::max(0.12f, fraction - 0.025f));
            }
        };
        restrictAtPlane(cameraTarget.x, eyeOffset.x, 9.45f, true);
        restrictAtPlane(cameraTarget.x, eyeOffset.x, -9.45f, false);
        restrictAtPlane(cameraTarget.z, eyeOffset.z, 7.40f, true);
        eyeOffset.x *= allowed;
        eyeOffset.y *= allowed;
        eyeOffset.z *= allowed;
    }

    Vec3 cameraEye{
        cameraTarget.x + eyeOffset.x,
        cameraTarget.y + eyeOffset.y,
        cameraTarget.z + eyeOffset.z
    };
    cameraEye.y = std::max(cameraEye.y, cameraTarget.y > -0.5f ? 0.28f : cameraEye.y);

    const Mat4 view = lookAtLH(cameraEye, cameraTarget, {0.0f, 1.0f, 0.0f});
    const Mat4 projection = perspectiveLH(
        50.0f * (kPi / 180.0f),
        static_cast<float>(width) / static_cast<float>(height),
        0.1f,
        100.0f
    );
    const Mat4 viewProjection = multiply(projection, view);

    auto drawModel = [&](const Mat4& model, Uint32 palette = Shell) {
        const Mat4 mvp = multiply(viewProjection, model);
        SDL_PushGPUVertexUniformData(commands, 0, mvp.m, sizeof(mvp.m));
        SDL_DrawGPUPrimitives(
            pass,
            kCubeVertexCount,
            1,
            palette * kCubeVertexCount,
            0
        );
    };

    auto drawBox = [&](const Vec3& position,
                       const Vec3& dimensions,
                       Uint32 palette = Shell) {
        drawModel(multiply(translation(position), scale(dimensions)), palette);
    };

    // The renderer consumes a semantic world description. Layout, repetition,
    // materials, and primitive roles live in the dependency-free world layer.
    for (const hakui::WorldPrimitive& primitive : worldGeometry) {
        for (std::uint16_t repeat = 0; repeat < primitive.repeatCount; ++repeat) {
            const float index = static_cast<float>(repeat);
            const Vec3 position{
                primitive.x + primitive.repeatX * index,
                primitive.y + primitive.repeatY * index,
                primitive.z + primitive.repeatZ * index
            };
            const Mat4 rotation = multiply(
                rotationY(primitive.rotationY),
                multiply(
                    rotationX(primitive.rotationX),
                    rotationZ(primitive.rotationZ)
                )
            );
            const Mat4 model = multiply(
                translation(position),
                multiply(rotation, scale({
                    primitive.width,
                    primitive.height,
                    primitive.depth
                }))
            );
            drawModel(model, paletteFor(primitive.material));
        }
    }

    // Runtime state decorates reusable geometry without owning its layout.
    drawBox({0.0f, 1.55f, -0.70f}, {1.08f, 0.66f, 0.035f},
            scene.terminalPowered ? TerminalGreen : Danger);
    if (scene.cardSuiteActive) {
        for (int card = 0; card < 4; ++card) {
            drawBox({-0.75f + card * 0.50f, 1.19f, 0.15f},
                    {0.30f, 0.035f, 0.46f}, card % 2 == 0 ? Shell : Danger);
        }
        for (int chip = 0; chip < 5; ++chip) {
            drawBox({1.25f, 1.20f + chip * 0.055f, 0.55f},
                    {0.30f, 0.055f, 0.30f}, chip % 2 == 0 ? Magenta : Cyan);
        }
    } else {
        drawBox({-0.32f, 1.31f, 0.20f}, {0.34f, 0.34f, 0.34f}, Shell);
        drawBox({0.32f, 1.31f, 0.20f}, {0.34f, 0.34f, 0.34f},
                scene.diceTotal > 0 ? Amber : Shell);
    }

    // HAKUI PROCEDURAL HUMANOID v0.65: acceleration-aware blending, turning,
    // airborne posture, and shared seated poses for furniture/table anchors.
    const bool seated = player.activity != PlayerActivity::Roaming;
    const float gait = std::sin(player.gaitPhase);
    const float counterGait = std::sin(player.gaitPhase + kPi);
    const float groundedBlend = player.grounded ? 1.0f : 0.20f;
    const float stride = 0.72f * player.movementBlend * groundedBlend;
    const float armStride = 0.82f * player.movementBlend * groundedBlend;
    const float idleBreath = 0.012f * std::sin(player.idlePhase);
    const float bodyBob =
        0.045f * std::abs(std::sin(player.gaitPhase)) * player.movementBlend * groundedBlend;
    const float bodySway = 0.035f * gait * player.movementBlend;
    const float airborneLean = player.grounded
        ? 0.0f
        : std::clamp(-player.velocityY * 0.035f, -0.18f, 0.30f);

    const bool playerKnockedDown = scene.combatActive &&
        scene.playerCombatState == hakui::combat::CombatState::KnockedDown;
    const Mat4 avatarRoot = multiply(
        translation({
            player.x,
            player.y + bodyBob + (playerKnockedDown ? 0.34f : 0.0f),
            player.z
        }),
        multiply(
            rotationY(player.yaw),
            rotationZ(playerKnockedDown ? 1.32f : 0.0f)
        )
    );

    auto localBox = [&](const Vec3& position,
                        const Vec3& dimensions,
                        Uint32 palette = Shell) {
        drawModel(multiply(
            avatarRoot,
            multiply(translation(position), scale(dimensions))
        ), palette);
    };

    auto hingedBox = [&](const Vec3& joint,
                         float angle,
                         const Vec3& centerOffset,
                         const Vec3& dimensions,
                         Uint32 palette = Shell) {
        const Mat4 hinge = multiply(
            avatarRoot,
            multiply(translation(joint), rotationX(angle))
        );
        drawModel(multiply(
            hinge,
            multiply(translation(centerOffset), scale(dimensions))
        ), palette);
    };

    auto leg = [&](float side, float angle, float kneeAngle) {
        const Vec3 hip{side * 0.23f, 1.17f, 0.0f};
        const Mat4 upper = multiply(
            avatarRoot,
            multiply(translation(hip), rotationX(angle))
        );
        drawModel(multiply(upper,
            multiply(translation({0.0f, -0.38f, 0.0f}),
                     scale({0.30f, 0.76f, 0.34f}))), Shell);

        const Mat4 lower = multiply(
            upper,
            multiply(translation({0.0f, -0.76f, 0.0f}), rotationX(kneeAngle))
        );
        drawModel(multiply(lower,
            multiply(translation({0.0f, -0.37f, 0.0f}),
                     scale({0.28f, 0.74f, 0.30f}))), Shell);
        drawModel(multiply(lower,
            multiply(translation({0.0f, -0.76f, 0.14f}),
                     scale({0.32f, 0.16f, 0.58f}))), Cyan);
    };

    auto arm = [&](float side, float angle) {
        hingedBox(
            {side * 0.60f, 2.12f, 0.0f},
            angle,
            {0.0f, -0.50f, 0.0f},
            {0.24f, 1.00f, 0.28f}
        );
    };

    if (seated) {
        leg(-1.0f, -1.28f, 1.26f);
        leg(1.0f, -1.28f, 1.26f);
    } else {
        leg(-1.0f, gait * stride, std::max(0.0f, -gait) * 0.58f);
        leg(1.0f, counterGait * stride, std::max(0.0f, -counterGait) * 0.58f);
    }
    localBox({0.0f, 1.20f, 0.0f}, {0.74f, 0.30f, 0.44f}, Midnight);

    const Mat4 torso = multiply(
        avatarRoot,
        multiply(
            translation({0.0f, 1.72f + idleBreath, 0.0f}),
            multiply(rotationX(airborneLean),
                multiply(rotationZ(bodySway), scale({0.92f, 0.94f, 0.48f})))
        )
    );
    drawModel(torso, scene.playerHitPulse > 0.0f ? Danger : Shell);

    float leftArmAngle = (seated ? -0.62f : 0.0f) + counterGait * armStride;
    float rightArmAngle = (seated ? -0.62f : 0.0f) + gait * armStride;
    if (scene.combatActive) {
        using hakui::combat::AttackSemantic;
        using hakui::combat::CombatState;
        if (scene.playerCombatState == CombatState::Guarding) {
            leftArmAngle = -1.48f;
            rightArmAngle = -1.48f;
        } else if (scene.playerCombatState == CombatState::Windup) {
            leftArmAngle = scene.playerAttack == AttackSemantic::Jab ? 0.46f : -1.10f;
            rightArmAngle = scene.playerAttack == AttackSemantic::Cross ? 0.62f : -1.10f;
        } else if (scene.playerCombatState == CombatState::Release) {
            leftArmAngle = scene.playerAttack == AttackSemantic::Jab ? -1.92f : -1.22f;
            rightArmAngle = scene.playerAttack == AttackSemantic::Cross ? -1.92f : -1.22f;
        } else if (scene.playerCombatState == CombatState::Staggered) {
            leftArmAngle = 0.52f;
            rightArmAngle = 0.20f;
        }
    }
    arm(-1.0f, leftArmAngle);
    arm(1.0f, rightArmAngle);
    localBox({0.0f, 2.28f + idleBreath, 0.0f}, {0.22f, 0.18f, 0.22f}, Cyan);
    localBox({0.0f, 2.60f + idleBreath, 0.0f}, {0.56f, 0.58f, 0.52f}, Shell);

    if (scene.combatActive) {
        using hakui::combat::AttackSemantic;
        using hakui::combat::CombatState;

        const bool opponentDown =
            scene.opponentCombatState == CombatState::KnockedDown;
        const Mat4 opponentRoot = multiply(
            translation({
                scene.opponentX,
                scene.opponentY + (opponentDown ? 0.34f : 0.0f),
                scene.opponentZ
            }),
            multiply(
                rotationY(scene.opponentYaw),
                rotationZ(opponentDown ? -1.32f : 0.0f)
            )
        );
        const Uint32 opponentBody =
            scene.opponentHitPulse > 0.0f ? Danger : Midnight;

        auto opponentBox = [&](const Vec3& position,
                               const Vec3& dimensions,
                               Uint32 palette) {
            drawModel(multiply(
                opponentRoot,
                multiply(translation(position), scale(dimensions))
            ), palette);
        };
        auto opponentArm = [&](float side, float angle) {
            const Mat4 hinge = multiply(
                opponentRoot,
                multiply(
                    translation({side * 0.60f, 2.12f, 0.0f}),
                    rotationX(angle)
                )
            );
            drawModel(multiply(
                hinge,
                multiply(
                    translation({0.0f, -0.50f, 0.0f}),
                    scale({0.24f, 1.00f, 0.28f})
                )
            ), opponentBody);
        };

        opponentBox({-0.23f, 0.78f, 0.0f}, {0.30f, 1.56f, 0.34f}, Shell);
        opponentBox({0.23f, 0.78f, 0.0f}, {0.30f, 1.56f, 0.34f}, Shell);
        opponentBox({0.0f, 1.72f, 0.0f}, {0.92f, 0.94f, 0.48f}, opponentBody);
        opponentBox({0.0f, 2.28f, 0.0f}, {0.22f, 0.18f, 0.22f}, Magenta);
        opponentBox({0.0f, 2.60f, 0.0f}, {0.56f, 0.58f, 0.52f}, Shell);

        float opponentLeftArm = -1.05f;
        float opponentRightArm = -1.05f;
        if (scene.opponentCombatState == CombatState::Guarding) {
            opponentLeftArm = -1.48f;
            opponentRightArm = -1.48f;
        } else if (scene.opponentCombatState == CombatState::Windup) {
            opponentLeftArm =
                scene.opponentAttack == AttackSemantic::Jab ? 0.46f : -1.10f;
            opponentRightArm =
                scene.opponentAttack == AttackSemantic::Cross ? 0.62f : -1.10f;
        } else if (scene.opponentCombatState == CombatState::Release) {
            opponentLeftArm =
                scene.opponentAttack == AttackSemantic::Jab ? -1.92f : -1.22f;
            opponentRightArm =
                scene.opponentAttack == AttackSemantic::Cross ? -1.92f : -1.22f;
        } else if (scene.opponentCombatState == CombatState::Staggered) {
            opponentLeftArm = 0.52f;
            opponentRightArm = 0.20f;
        }
        opponentArm(-1.0f, opponentLeftArm);
        opponentArm(1.0f, opponentRightArm);
    }

    if (scene.paused) {
        // Physical pause beacon; the detailed controls/status remain in the
        // native HUD title until the dedicated glyph atlas lands.
        drawBox({0.0f, 3.25f, 2.2f}, {4.8f, 0.12f, 0.12f}, Danger);
        drawBox({0.0f, 2.85f, 2.2f}, {2.8f, 0.08f, 0.08f}, Amber);
    }

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
