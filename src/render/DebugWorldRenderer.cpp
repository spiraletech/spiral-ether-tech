#include "render/DebugWorldRenderer.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include "avatar/AvatarGroundContact.hpp"
#include "avatar/RideAttachmentRig.hpp"
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
        const float shoulderOffset = 0.58f * cameraRig_.shoulderSide();
        desiredTargetX += -std::cos(cameraRig_.yaw()) * shoulderOffset;
        desiredTargetZ += std::sin(cameraRig_.yaw()) * shoulderOffset;
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
    cameraYaw_ = smoothToward(cameraYaw_, cameraRig_.yaw(), 14.0f, deltaSeconds);
    cameraPitch_ = smoothToward(cameraPitch_, cameraRig_.pitch(), 14.0f, deltaSeconds);
    cameraDistance_ = smoothToward(
        cameraDistance_,
        cameraRig_.distance(),
        12.0f,
        deltaSeconds
    );
}

void DebugWorldRenderer::orbitCamera(float horizontalPixels, float verticalPixels)
{
    cameraRig_.orbit(horizontalPixels, verticalPixels);
}

void DebugWorldRenderer::zoomCamera(float wheelSteps)
{
    cameraRig_.zoom(wheelSteps);
}

void DebugWorldRenderer::resetCamera()
{
    cameraRig_.reset();
}

void DebugWorldRenderer::toggleShoulder()
{
    cameraRig_.toggleShoulder();
}

void DebugWorldRenderer::adjustLookSensitivity(float delta)
{
    cameraRig_.adjustLookSensitivity(delta);
}

float DebugWorldRenderer::lookSensitivity() const noexcept
{
    return cameraRig_.lookSensitivity();
}

float DebugWorldRenderer::cameraYaw() const noexcept
{
    return cameraRig_.yaw();
}

float DebugWorldRenderer::cameraPitch() const noexcept
{
    return cameraRig_.pitch();
}

float DebugWorldRenderer::cameraDistance() const noexcept
{
    return cameraRig_.distance();
}

float DebugWorldRenderer::cameraShoulderSide() const noexcept
{
    return cameraRig_.shoulderSide();
}

float DebugWorldRenderer::cameraWorldX() const noexcept { return cameraEyeX_; }
float DebugWorldRenderer::cameraWorldY() const noexcept { return cameraEyeY_; }
float DebugWorldRenderer::cameraWorldZ() const noexcept { return cameraEyeZ_; }
float DebugWorldRenderer::cameraTargetX() const noexcept { return cameraTargetX_; }
float DebugWorldRenderer::cameraTargetY() const noexcept { return cameraTargetY_; }
float DebugWorldRenderer::cameraTargetZ() const noexcept { return cameraTargetZ_; }
float DebugWorldRenderer::fieldOfViewDegrees() const noexcept { return 50.0f; }

void DebugWorldRenderer::setCameraRole(CameraRole role)
{
    if (cameraRole_ == role) {
        return;
    }

    cameraRole_ = role;
    if (role == CameraRole::CombatFrame || role == CameraRole::TargetFrame ||
        role == CameraRole::DuelFrame) {
        cameraRig_.setFraming(3.12f, 0.38f, 7.2f);
    } else if (role == CameraRole::InteractionFrame &&
               interactionFrame_ == InteractionFrame::FusionTable) {
        cameraRig_.setFraming(0.0f, 0.62f, 5.6f);
    } else if (role == CameraRole::InteractionFrame &&
               interactionFrame_ == InteractionFrame::LoungeCouch) {
        cameraRig_.setFraming(1.85f, 0.46f, 5.0f);
    } else {
        cameraRig_.setFraming(cameraRig_.yaw(), cameraRig_.pitch(), 8.5f);
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
    return cameraRig_.yaw();
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

    SDL_Log("[HAKUI] v0.83 DATA GRUNGE renderer // pop then flick rhythm online");
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
    if (cameraTarget.y > -0.5f && cameraTarget.z < 8.0f) {
        // The proportional sweep above preserves camera distance where it can,
        // while this final interior-face cap handles the degenerate case where
        // a smoothed target is already on a shell boundary. Without this cap,
        // the minimum sweep fraction can leave the eye inside the wall.
        cameraEye.x = std::clamp(cameraEye.x, -9.45f, 9.45f);
        cameraEye.z = std::min(cameraEye.z, 7.40f);
    }
    cameraEyeX_ = cameraEye.x;
    cameraEyeY_ = cameraEye.y;
    cameraEyeZ_ = cameraEye.z;

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

    // Locomotion embodiment is presentation driven by deterministic player
    // state. These procedural entities contain no movement or trick rules.
    const bool ridingSkateboard =
        player.locomotion == LocomotionMode::Skateboard &&
        player.activity == PlayerActivity::Roaming;
    const bool ridingBmx =
        player.locomotion == LocomotionMode::BMX &&
        player.activity == PlayerActivity::Roaming;
    float ridePitch = 0.0f;
    float rideRoll = 0.0f;
    if (scene.rideable.phase == hakui::RidePhase::Manual) {
        ridePitch = ridingSkateboard ? -0.20f : -0.32f;
    } else if (scene.rideable.phase == hakui::RidePhase::Grinding) {
        rideRoll = ridingSkateboard ? 0.12f : -0.09f;
    } else if (scene.rideable.phase == hakui::RidePhase::Crash) {
        rideRoll = ridingSkateboard ? 0.72f : -0.62f;
    }
    if (scene.rideable.phase == hakui::RidePhase::Airborne) {
        if (scene.rideable.activeTrick == hakui::RideTrick::Kickflip) {
            rideRoll = scene.rideable.phaseSeconds * 10.5f;
        } else if (scene.rideable.activeTrick == hakui::RideTrick::Heelflip) {
            rideRoll = -scene.rideable.phaseSeconds * 10.5f;
        } else if (scene.rideable.activeTrick == hakui::RideTrick::BmxTabletop) {
            rideRoll = std::sin(scene.rideable.phaseSeconds * 4.5f) * 0.72f;
        }
    }
    const Mat4 locomotionRoot = multiply(
        translation({player.x, player.y, player.z}),
        multiply(
            rotationY(player.yaw),
            multiply(rotationZ(rideRoll), rotationX(ridePitch))
        )
    );
    auto locomotionModel = [&](const Mat4& local, Uint32 palette) {
        drawModel(multiply(locomotionRoot, local), palette);
    };
    auto locomotionBox = [&](const Vec3& position,
                             const Vec3& dimensions,
                             Uint32 palette) {
        locomotionModel(
            multiply(translation(position), scale(dimensions)),
            palette
        );
    };

    if (ridingSkateboard) {
        locomotionBox({0.0f, 0.18f, 0.0f}, {0.76f, 0.10f, 1.72f}, Magenta);
        locomotionBox({0.0f, 0.10f, -0.55f}, {0.92f, 0.08f, 0.14f}, Cyan);
        locomotionBox({0.0f, 0.10f, 0.55f}, {0.92f, 0.08f, 0.14f}, Cyan);
        for (float side : {-1.0f, 1.0f}) {
            for (float end : {-1.0f, 1.0f}) {
                const Vec3 wheelCenter{side * 0.48f, 0.10f, end * 0.56f};
                const Mat4 wheel = multiply(
                    translation(wheelCenter),
                    multiply(
                        rotationX(player.gaitPhase * 0.75f),
                        scale({0.16f, 0.27f, 0.27f})
                    )
                );
                locomotionModel(wheel, Midnight);
                const Mat4 wheelStripe = multiply(
                    translation(wheelCenter),
                    multiply(
                        rotationX(player.gaitPhase * 0.75f),
                        scale({0.18f, 0.055f, 0.23f})
                    )
                );
                locomotionModel(wheelStripe, Amber);
            }
        }
    }

    if (ridingBmx) {
        constexpr float wheelRadius = 0.62f;
        constexpr int wheelSegments = 12;
        const float steering = scene.rideable.steeringVisual * 0.48f;
        static const hakui::RideAttachmentRig bmxRig =
            hakui::RideAttachmentRig::bmx();
        const auto anchorPosition = [&](
            hakui::RideAnchorSemantic semantic,
            const Vec3& fallback
        ) {
            const hakui::RideAnchor* anchor = bmxRig.find(semantic);
            return anchor ? Vec3{anchor->x, anchor->y, anchor->z} : fallback;
        };
        const Vec3 rearHub = anchorPosition(
            hakui::RideAnchorSemantic::RearAxle,
            {0.0f, 0.65f, -0.88f}
        );
        const Vec3 frontHub = anchorPosition(
            hakui::RideAnchorSemantic::FrontAxle,
            {0.0f, 0.65f, 0.88f}
        );
        const Vec3 leftGrip = anchorPosition(
            hakui::RideAnchorSemantic::LeftHandGrip,
            {-0.62f, 1.53f, 0.68f}
        );
        const Vec3 rightGrip = anchorPosition(
            hakui::RideAnchorSemantic::RightHandGrip,
            {0.62f, 1.53f, 0.68f}
        );
        const Vec3 leftPedal = anchorPosition(
            hakui::RideAnchorSemantic::LeftFootAnchor,
            {-0.28f, 0.66f, 0.05f}
        );
        const Vec3 rightPedal = anchorPosition(
            hakui::RideAnchorSemantic::RightFootAnchor,
            {0.28f, 0.66f, 0.05f}
        );

        // The wheel and every steering component share one local assembly.
        // +Z is bicycle-forward, so the front root must have greater Z than
        // the rear root in every camera view.
        const Mat4 rearAssembly = translation(rearHub);
        const Mat4 frontAssembly = multiply(
            translation(frontHub),
            rotationY(steering)
        );
        auto bikeWheel = [&](const Mat4& assembly) {
            for (int segment = 0; segment < wheelSegments; ++segment) {
                const float angle =
                    (2.0f * kPi * static_cast<float>(segment) /
                     static_cast<float>(wheelSegments)) +
                    player.gaitPhase * 0.42f;
                const Mat4 tire = multiply(
                    assembly,
                    multiply(
                        rotationX(angle),
                        multiply(
                            translation({0.0f, wheelRadius, 0.0f}),
                            scale({0.13f, 0.11f, 0.34f})
                        )
                    )
                );
                locomotionModel(tire, Midnight);
            }
            locomotionModel(multiply(assembly, scale({0.22f, 0.22f, 0.22f})), Amber);
            for (int spoke = 0; spoke < 4; ++spoke) {
                const float angle =
                    kPi * static_cast<float>(spoke) * 0.25f +
                    player.gaitPhase * 0.42f;
                locomotionModel(
                    multiply(
                        assembly,
                        multiply(
                            rotationX(angle),
                            scale({0.045f, wheelRadius * 1.75f, 0.045f})
                        )
                    ),
                    Cyan
                );
            }
        };

        auto frameBar = [&](const Vec3& from,
                            const Vec3& to,
                            float width,
                            Uint32 palette) {
            const float deltaY = to.y - from.y;
            const float deltaZ = to.z - from.z;
            const float length = std::sqrt(deltaY * deltaY + deltaZ * deltaZ);
            const Vec3 midpoint{
                (from.x + to.x) * 0.5f,
                (from.y + to.y) * 0.5f,
                (from.z + to.z) * 0.5f
            };
            const float angle = std::atan2(deltaZ, deltaY);
            locomotionModel(
                multiply(
                    translation(midpoint),
                    multiply(rotationX(angle), scale({width, length, width}))
                ),
                palette
            );
        };

        bikeWheel(rearAssembly);
        bikeWheel(frontAssembly);
        const Vec3 crank{0.0f, 0.66f, 0.05f};
        const Vec3 seatPost{0.0f, 1.30f, -0.30f};
        const Vec3 headTube{0.0f, 1.18f, 0.58f};
        frameBar(rearHub, crank, 0.10f, Magenta);
        frameBar(rearHub, seatPost, 0.10f, Magenta);
        frameBar(seatPost, crank, 0.10f, Magenta);
        frameBar(seatPost, headTube, 0.10f, Cyan);
        frameBar(headTube, crank, 0.10f, Cyan);
        frameBar(headTube, frontHub, 0.085f, Shell);
        locomotionBox({0.0f, 1.36f, -0.32f}, {0.48f, 0.11f, 0.32f}, Shell);
        for (float forkSide : {-1.0f, 1.0f}) {
            locomotionModel(
                multiply(
                    frontAssembly,
                    multiply(
                        translation({forkSide * 0.16f, 0.29f, -0.15f}),
                        multiply(rotationX(0.48f), scale({0.065f, 0.68f, 0.065f}))
                    )
                ),
                Shell
            );
        }
        locomotionModel(
            multiply(
                frontAssembly,
                multiply(
                    translation({0.0f, 0.68f, -0.23f}),
                    multiply(rotationX(0.18f), scale({0.10f, 0.42f, 0.10f}))
                )
            ),
            Magenta
        );
        locomotionModel(
            multiply(
                frontAssembly,
                multiply(
                    translation({0.0f, leftGrip.y - frontHub.y, leftGrip.z - frontHub.z}),
                    scale({1.18f, 0.09f, 0.10f})
                )
            ),
            Cyan
        );
        for (const Vec3& grip : {leftGrip, rightGrip}) {
            locomotionModel(
                multiply(
                    frontAssembly,
                    multiply(
                        translation({
                            grip.x - frontHub.x,
                            grip.y - frontHub.y,
                            grip.z - frontHub.z
                        }),
                        scale({0.20f, 0.13f, 0.15f})
                    )
                ),
                Midnight
            );
        }
        locomotionBox(crank, {0.28f, 0.28f, 0.12f}, Amber);
        locomotionBox(leftPedal, {0.42f, 0.07f, 0.13f}, Shell);
        locomotionBox(rightPedal, {0.42f, 0.07f, 0.13f}, Shell);
    }

    // HAKUI PROCEDURAL HUMANOID: acceleration-aware blending, turning,
    // airborne posture, and shared seated poses for furniture/table anchors.
    const bool seated = player.activity != PlayerActivity::Roaming;
    const bool mounted = ridingSkateboard || ridingBmx;
    const float gait = std::sin(player.gaitPhase);
    const float counterGait = std::sin(player.gaitPhase + kPi);
    const float groundedBlend = player.grounded ? 1.0f : 0.20f;
    const float stride = 0.72f * player.movementBlend * groundedBlend;
    const float armStride = 0.82f * player.movementBlend * groundedBlend;
    const float idleBreath = 0.012f * std::sin(player.idlePhase);
    const float bodyBob = mounted
        ? 0.0f
        : 0.045f * std::abs(std::sin(player.gaitPhase)) *
            player.movementBlend * groundedBlend;
    const float bodySway = 0.035f * gait * player.movementBlend;
    const float airborneLean = player.grounded
        ? 0.0f
        : std::clamp(-player.velocityY * 0.035f, -0.18f, 0.30f);
    const float expressiveRideLean = scene.rideable.phase == hakui::RidePhase::Manual
        ? (ridingSkateboard ? -0.16f : -0.24f)
        : (scene.rideable.phase == hakui::RidePhase::Grinding ? 0.09f : 0.0f);
    const float expressiveRideSway = scene.rideable.phase == hakui::RidePhase::Grinding
        ? scene.rideable.balanceOffset * 0.22f
        : 0.0f;

    const bool playerKnockedDown = scene.combatActive &&
        scene.playerCombatState == hakui::combat::CombatState::KnockedDown;
    hakui::EmbodimentProfileId embodiment =
        hakui::EmbodimentProfileId::OnFoot;
    if (playerKnockedDown) {
        embodiment = hakui::EmbodimentProfileId::Knockdown;
    } else if (scene.combatActive) {
        embodiment = hakui::EmbodimentProfileId::Combat;
    } else if (seated) {
        embodiment = hakui::EmbodimentProfileId::Seated;
    } else if (ridingSkateboard) {
        embodiment = hakui::EmbodimentProfileId::Skateboard;
    } else if (ridingBmx) {
        embodiment = hakui::EmbodimentProfileId::Bmx;
    }
    const hakui::AvatarGroundContactProfile& groundContact =
        hakui::avatarGroundContactProfile(embodiment);
    const Mat4 avatarRoot = multiply(
        translation({
            player.x,
            player.y + bodyBob + groundContact.visualRootAbovePlayerBase,
            player.z
        }),
        multiply(
            rotationY(player.yaw),
            rotationZ(
                playerKnockedDown
                    ? 1.32f
                    : (scene.rideable.phase == hakui::RidePhase::Crash ? 0.92f : 0.0f)
            )
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
    } else if (ridingSkateboard) {
        const float boardKnee = scene.rideable.phase == hakui::RidePhase::Airborne
            ? 0.72f
            : (scene.rideable.phase == hakui::RidePhase::Manual ? 0.48f : 0.30f);
        leg(-1.0f, -0.22f - expressiveRideLean, boardKnee + 0.04f);
        leg(1.0f, 0.18f - expressiveRideLean, boardKnee);
    } else if (ridingBmx) {
        const float pedal = std::sin(player.gaitPhase * 0.42f) * 0.28f;
        const float hopTuck = scene.rideable.phase == hakui::RidePhase::Airborne
            ? 0.26f
            : 0.0f;
        leg(-1.0f, -0.62f + pedal - hopTuck, 1.05f - pedal * 0.5f + hopTuck);
        leg(1.0f, -0.62f - pedal - hopTuck, 1.05f + pedal * 0.5f + hopTuck);
    } else if (scene.combatActive) {
        const float stanceStep = std::sin(player.gaitPhase) *
            0.18f * scene.playerStanceBlend;
        leg(-1.0f, -0.10f + stanceStep, 0.22f);
        leg(1.0f, 0.14f - stanceStep, 0.28f);
    } else {
        leg(-1.0f, gait * stride, std::max(0.0f, -gait) * 0.58f);
        leg(1.0f, counterGait * stride, std::max(0.0f, -counterGait) * 0.58f);
    }
    localBox({0.0f, 1.20f, 0.0f}, {0.74f, 0.30f, 0.44f}, Midnight);

    const bool attackRelease = scene.combatActive &&
        scene.playerCombatState == hakui::combat::CombatState::Release;
    const float attackCommitment = attackRelease
        ? (scene.playerAttack == hakui::combat::AttackSemantic::Cross ? 0.28f : 0.18f)
        : 0.0f;
    const float impactLean = scene.playerCombatState ==
        hakui::combat::CombatState::Staggered ? -0.22f : 0.0f;
    const Mat4 torso = multiply(
        avatarRoot,
        multiply(
            translation({0.0f, 1.72f + idleBreath, attackCommitment}),
            multiply(
                rotationX(
                    airborneLean + expressiveRideLean +
                    (ridingBmx ? 0.20f : 0.0f)
                ),
                multiply(
                    rotationZ(bodySway + expressiveRideSway + impactLean),
                    scale({0.92f, 0.94f, 0.48f})
                )
            )
        )
    );
    drawModel(torso, scene.playerHitPulse > 0.0f ? Danger : Shell);

    float leftArmAngle = (seated ? -0.62f : 0.0f) + counterGait * armStride;
    float rightArmAngle = (seated ? -0.62f : 0.0f) + gait * armStride;
    if (ridingSkateboard) {
        leftArmAngle = -0.28f;
        rightArmAngle = 0.22f;
    } else if (ridingBmx) {
        leftArmAngle = -1.06f - scene.rideable.steeringVisual * 0.10f;
        rightArmAngle = -1.06f + scene.rideable.steeringVisual * 0.10f;
    } else if (scene.combatActive) {
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

    if (scene.sparDummyVisible) {
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
                multiply(
                    translation({0.0f, 0.0f, -scene.opponentHitPulse * 2.1f}),
                    rotationZ(
                        opponentDown
                            ? -1.32f
                            : (scene.opponentCombatState == CombatState::Staggered
                                ? 0.24f
                                : 0.0f)
                    )
                )
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
