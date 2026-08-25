#include "render/DebugWorldRenderer.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <string>
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

using GlyphRows = std::array<std::uint8_t, 7>;

GlyphRows glyphRows(char source) noexcept
{
    const char character = source;
    switch (character) {
    case 'a': return {0,0,14,1,15,17,15};
    case 'b': return {16,16,30,17,17,17,30};
    case 'c': return {0,0,14,17,16,17,14};
    case 'd': return {1,1,15,17,17,17,15};
    case 'e': return {0,0,14,17,31,16,14};
    case 'f': return {6,9,8,28,8,8,8};
    case 'g': return {0,0,15,17,15,1,14};
    case 'h': return {16,16,30,17,17,17,17};
    case 'i': return {4,0,12,4,4,4,14};
    case 'j': return {2,0,6,2,2,18,12};
    case 'k': return {16,16,18,20,24,20,18};
    case 'l': return {12,4,4,4,4,4,14};
    case 'm': return {0,0,26,21,21,17,17};
    case 'n': return {0,0,30,17,17,17,17};
    case 'o': return {0,0,14,17,17,17,14};
    case 'p': return {0,0,30,17,30,16,16};
    case 'q': return {0,0,15,17,15,1,1};
    case 'r': return {0,0,22,25,16,16,16};
    case 's': return {0,0,15,16,14,1,30};
    case 't': return {8,8,28,8,8,9,6};
    case 'u': return {0,0,17,17,17,19,13};
    case 'v': return {0,0,17,17,17,10,4};
    case 'w': return {0,0,17,17,21,21,10};
    case 'x': return {0,0,17,10,4,10,17};
    case 'y': return {0,0,17,17,15,1,14};
    case 'z': return {0,0,31,2,4,8,31};
    case 'A': return {14,17,17,31,17,17,17};
    case 'B': return {30,17,17,30,17,17,30};
    case 'C': return {14,17,16,16,16,17,14};
    case 'D': return {30,17,17,17,17,17,30};
    case 'E': return {31,16,16,30,16,16,31};
    case 'F': return {31,16,16,30,16,16,16};
    case 'G': return {14,17,16,23,17,17,15};
    case 'H': return {17,17,17,31,17,17,17};
    case 'I': return {31,4,4,4,4,4,31};
    case 'J': return {7,2,2,2,18,18,12};
    case 'K': return {17,18,20,24,20,18,17};
    case 'L': return {16,16,16,16,16,16,31};
    case 'M': return {17,27,21,21,17,17,17};
    case 'N': return {17,25,21,19,17,17,17};
    case 'O': return {14,17,17,17,17,17,14};
    case 'P': return {30,17,17,30,16,16,16};
    case 'Q': return {14,17,17,17,21,18,13};
    case 'R': return {30,17,17,30,20,18,17};
    case 'S': return {15,16,16,14,1,1,30};
    case 'T': return {31,4,4,4,4,4,4};
    case 'U': return {17,17,17,17,17,17,14};
    case 'V': return {17,17,17,17,17,10,4};
    case 'W': return {17,17,17,21,21,21,10};
    case 'X': return {17,17,10,4,10,17,17};
    case 'Y': return {17,17,10,4,4,4,4};
    case 'Z': return {31,1,2,4,8,16,31};
    case '0': return {14,17,19,21,25,17,14};
    case '1': return {4,12,4,4,4,4,14};
    case '2': return {14,17,1,2,4,8,31};
    case '3': return {30,1,1,14,1,1,30};
    case '4': return {2,6,10,18,31,2,2};
    case '5': return {31,16,16,30,1,1,30};
    case '6': return {14,16,16,30,17,17,14};
    case '7': return {31,1,2,4,8,8,8};
    case '8': return {14,17,17,14,17,17,14};
    case '9': return {14,17,17,15,1,1,14};
    case '?': return {14,17,1,2,4,0,4};
    case '!': return {4,4,4,4,4,0,4};
    case '.': return {0,0,0,0,0,6,6};
    case ',': return {0,0,0,0,6,6,4};
    case ':': return {0,6,6,0,6,6,0};
    case '-': return {0,0,0,31,0,0,0};
    case '_': return {0,0,0,0,0,0,31};
    case '>': return {16,8,4,2,4,8,16};
    case '/': return {1,2,2,4,8,8,16};
    case '\'': return {4,4,2,0,0,0,0};
    default: return {};
    }
}

std::string fontDisplayText(std::string_view utf8, std::size_t maximum)
{
    std::string output;
    output.reserve(std::min(maximum, utf8.size()));
    std::size_t offset = 0;
    while (offset < utf8.size() && output.size() < maximum) {
        const unsigned char lead = static_cast<unsigned char>(utf8[offset]);
        if (lead < 0x80u) {
            output.push_back(lead >= 0x20u && lead <= 0x7eu
                ? static_cast<char>(lead) : '?');
            ++offset;
            continue;
        }
        std::size_t length = (lead & 0xe0u) == 0xc0u ? 2u
            : (lead & 0xf0u) == 0xe0u ? 3u
            : (lead & 0xf8u) == 0xf0u ? 4u : 1u;
        output.push_back('?');
        offset = std::min(offset + length, utf8.size());
    }
    if (offset < utf8.size() && maximum >= 3) {
        if (output.size() > maximum - 3) {
            output.resize(maximum - 3);
        }
        output += "...";
    }
    return output;
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

const BubbleVisualTelemetry&
DebugWorldRenderer::bubbleVisualTelemetry() const noexcept
{
    return bubbleVisualTelemetry_;
}

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

    SDL_Log("[HAKUI] v0.861 DATA GRUNGE renderer // translucent social glass online");
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

    // The bootstrap fragment shader emits alpha=1. A dedicated social-glass
    // pipeline therefore uses the GPU blend constant as authored opacity.
    // This gives the bubble real destination-color transparency without
    // coupling alpha or social policy to the shared world vertex format.
    colorTarget.blend_state.enable_blend = true;
    colorTarget.blend_state.color_write_mask = 0xF;
    colorTarget.blend_state.color_blend_op = SDL_GPU_BLENDOP_ADD;
    colorTarget.blend_state.alpha_blend_op = SDL_GPU_BLENDOP_ADD;
    colorTarget.blend_state.src_color_blendfactor =
        SDL_GPU_BLENDFACTOR_CONSTANT_COLOR;
    colorTarget.blend_state.dst_color_blendfactor =
        SDL_GPU_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR;
    colorTarget.blend_state.src_alpha_blendfactor =
        SDL_GPU_BLENDFACTOR_CONSTANT_COLOR;
    colorTarget.blend_state.dst_alpha_blendfactor =
        SDL_GPU_BLENDFACTOR_ONE_MINUS_CONSTANT_COLOR;
    pipelineInfo.depth_stencil_state.enable_depth_write = false;
    glassPipeline_ = SDL_CreateGPUGraphicsPipeline(device_, &pipelineInfo);

    SDL_ReleaseGPUShader(device_, vertexShader);
    SDL_ReleaseGPUShader(device_, fragmentShader);

    return pipeline_ != nullptr && glassPipeline_ != nullptr;
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

    bubbleVisualTelemetry_ = {};

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

    auto drawWorldText = [&](std::string_view text,
                             const Mat4& root,
                             float cellWidth,
                             float cellHeight,
                             float depth,
                             std::size_t charactersPerLine,
                             Uint32 palette) {
        float cursorUnits = 0.0f;
        std::size_t characterCount = 0;
        std::size_t line = 0;
        for (std::size_t index = 0; index < text.size(); ++index) {
            const unsigned char byte = static_cast<unsigned char>(text[index]);
            if ((byte & 0xc0u) == 0x80u) {
                continue;
            }
            const char character = byte < 0x80u ? text[index] : '?';
            if (character == '\n' || characterCount >= charactersPerLine) {
                ++line;
                cursorUnits = 0.0f;
                characterCount = 0;
                if (character == '\n') {
                    continue;
                }
            }
            const GlyphRows rows = glyphRows(character);
            for (std::size_t row = 0; row < rows.size(); ++row) {
                for (std::size_t bit = 0; bit < 5; ++bit) {
                    if ((rows[row] & (1u << (4u - bit))) == 0) {
                        continue;
                    }
                    const float x = (cursorUnits +
                        static_cast<float>(bit) + 0.5f) * cellWidth;
                    const float y = -(
                        static_cast<float>(line * 9 + row) + 0.5f
                    ) * cellHeight;
                    drawModel(
                        multiply(
                            root,
                            multiply(
                                translation({x, y, 0.0f}),
                                scale({cellWidth * 0.82f,
                                       cellHeight * 0.82f,
                                       depth})
                            )
                        ),
                        palette
                    );
                }
            }
            cursorUnits += hakui::social::ChatBubblePresentation::
                glyphAdvanceUnits(character);
            ++characterCount;
        }
    };

    auto drawClipModel = [&](const Mat4& clipModel, Uint32 palette) {
        SDL_PushGPUVertexUniformData(
            commands, 0, clipModel.m, sizeof(clipModel.m)
        );
        SDL_DrawGPUPrimitives(
            pass,
            kCubeVertexCount,
            1,
            palette * kCubeVertexCount,
            0
        );
    };

    auto drawClipText = [&](std::string_view text,
                            float originX,
                            float originY,
                            float cellWidth,
                            float cellHeight,
                            Uint32 palette) {
        float cursorUnits = 0.0f;
        for (std::size_t index = 0; index < text.size(); ++index) {
            const unsigned char byte = static_cast<unsigned char>(text[index]);
            if ((byte & 0xc0u) == 0x80u) {
                continue;
            }
            const char character = byte < 0x80u ? text[index] : '?';
            const GlyphRows rows = glyphRows(character);
            for (std::size_t row = 0; row < rows.size(); ++row) {
                for (std::size_t bit = 0; bit < 5; ++bit) {
                    if ((rows[row] & (1u << (4u - bit))) == 0) {
                        continue;
                    }
                    const float x = originX +
                        (cursorUnits + static_cast<float>(bit)) * cellWidth;
                    const float y = originY -
                        static_cast<float>(row) * cellHeight;
                    drawClipModel(
                        multiply(
                            translation({x, y, 0.005f}),
                            scale({cellWidth * 0.84f,
                                   cellHeight * 0.84f,
                                   0.002f})
                        ),
                        palette
                    );
                }
            }
            cursorUnits += hakui::social::ChatBubblePresentation::
                glyphAdvanceUnits(character);
        }
    };

    const auto bindGlass = [&](float alpha) {
        SDL_BindGPUGraphicsPipeline(pass, glassPipeline_);
        const float opacity = std::clamp(alpha, 0.0f, 1.0f);
        SDL_SetGPUBlendConstants(
            pass,
            SDL_FColor{opacity, opacity, opacity, opacity}
        );
    };

    const auto bindOpaque = [&]() {
        SDL_BindGPUGraphicsPipeline(pass, pipeline_);
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
    static const hakui::RideAttachmentRig skateboardRig =
        hakui::RideAttachmentRig::skateboard();
    static const hakui::RideAttachmentRig bmxRig =
        hakui::RideAttachmentRig::bmx();
    float ridePitch = 0.0f;
    float rideYaw = 0.0f;
    float rideRoll = 0.0f;
    if (scene.rideable.phase == hakui::RidePhase::Manual) {
        ridePitch = ridingSkateboard ? -0.20f : -0.32f;
    } else if (scene.rideable.phase == hakui::RidePhase::Grinding) {
        rideRoll = ridingSkateboard ? 0.12f : -0.09f;
    } else if (scene.rideable.phase == hakui::RidePhase::Crash) {
        rideRoll = ridingSkateboard ? 0.20f : -0.18f;
    }
    const bool boardRotation = ridingSkateboard &&
        scene.rideable.rotationChannel == hakui::RideRotationChannel::BoardDeck;
    const bool wholeRideRotation =
        scene.rideable.rotationChannel == hakui::RideRotationChannel::Rideable;
    if (boardRotation || wholeRideRotation ||
        scene.rideable.phase == hakui::RidePhase::Crash) {
        ridePitch += scene.rideable.rideableRotation.x;
        rideYaw += scene.rideable.rideableRotation.y;
        rideRoll += scene.rideable.rideableRotation.z;
    }
    const Mat4 locomotionRoot = multiply(
        translation({
            player.x + scene.rideable.rideSeparation,
            player.y,
            player.z
        }),
        multiply(
            rotationY(player.yaw),
            multiply(
                rotationY(rideYaw),
                multiply(rotationZ(rideRoll), rotationX(ridePitch))
            )
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
        const float trickSteering =
            scene.rideable.rotationChannel == hakui::RideRotationChannel::BmxSteering
                ? scene.rideable.rideableRotation.y
                : 0.0f;
        const float steering = scene.rideable.steeringVisual * 0.48f +
            trickSteering;
        const float crankRotation = player.gaitPhase * 0.42f +
            (scene.rideable.rotationChannel == hakui::RideRotationChannel::BmxCrank
                ? scene.rideable.rideableRotation.x
                : 0.0f);
        const float frameYaw =
            scene.rideable.rotationChannel == hakui::RideRotationChannel::BmxFrame
                ? scene.rideable.rideableRotation.y
                : 0.0f;
        const auto anchorPosition = [&](
            hakui::RideAnchorSemantic semantic,
            const Vec3& fallback,
            float anchorSteering = 0.0f,
            float anchorCrank = 0.0f
        ) {
            if (!bmxRig.find(semantic)) {
                return fallback;
            }
            const hakui::RideAnchorPosition anchor = bmxRig.resolvePosition(
                semantic,
                anchorSteering,
                anchorCrank
            );
            return Vec3{anchor.x, anchor.y, anchor.z};
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
            hakui::RideAnchorSemantic::LeftPedal,
            {-0.28f, 0.66f, 0.05f},
            0.0f,
            crankRotation
        );
        const Vec3 rightPedal = anchorPosition(
            hakui::RideAnchorSemantic::RightPedal,
            {0.28f, 0.66f, 0.05f},
            0.0f,
            crankRotation
        );

        // The wheel and every steering component share one local assembly.
        // +Z is bicycle-forward, so the front root must have greater Z than
        // the rear root in every camera view.
        const Mat4 bmxFrameTransform = multiply(
            translation(frontHub),
            multiply(
                rotationY(frameYaw),
                translation({-frontHub.x, -frontHub.y, -frontHub.z})
            )
        );
        const Mat4 rearAssembly = multiply(
            bmxFrameTransform,
            translation(rearHub)
        );
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
                    bmxFrameTransform,
                    multiply(
                    translation(midpoint),
                    multiply(rotationX(angle), scale({width, length, width}))
                    )
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
        locomotionModel(
            multiply(
                bmxFrameTransform,
                multiply(
                    translation({0.0f, 1.36f, -0.32f}),
                    scale({0.48f, 0.11f, 0.32f})
                )
            ),
            Shell
        );
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
        locomotionModel(
            multiply(bmxFrameTransform,
                     multiply(translation(crank), scale({0.28f, 0.28f, 0.12f}))),
            Amber
        );
        locomotionModel(
            multiply(bmxFrameTransform,
                     multiply(translation(leftPedal), scale({0.42f, 0.07f, 0.13f}))),
            Shell
        );
        locomotionModel(
            multiply(bmxFrameTransform,
                     multiply(translation(rightPedal), scale({0.42f, 0.07f, 0.13f}))),
            Shell
        );
    }

    // HAKUI PROCEDURAL HUMANOID: acceleration-aware blending, turning,
    // airborne posture, and shared seated poses for furniture/table anchors.
    const bool seated = player.activity != PlayerActivity::Roaming;
    const bool rideBail = (ridingSkateboard || ridingBmx) &&
        scene.rideable.phase == hakui::RidePhase::Crash;
    const bool mounted = (ridingSkateboard || ridingBmx) && !rideBail;
    const bool socialAllowed = !mounted && !rideBail && !scene.combatActive;
    const float socialWeight = socialAllowed
        ? std::clamp(scene.socialGestureWeight, 0.0f, 1.0f)
        : 0.0f;
    float socialTorsoPitch = 0.0f;
    float socialTorsoRoll = 0.0f;
    float socialHeadPitch = 0.0f;
    float socialHeadRoll = 0.0f;
    using hakui::social::SocialGesture;
    switch (scene.socialGesture) {
    case SocialGesture::Nod:
        socialHeadPitch = 0.16f * socialWeight;
        break;
    case SocialGesture::HeadTilt:
        socialHeadRoll = 0.13f * socialWeight;
        break;
    case SocialGesture::DisagreementTilt:
        socialHeadRoll = -0.14f * socialWeight;
        socialTorsoRoll = -0.025f * socialWeight;
        break;
    case SocialGesture::LaughPulse:
        socialHeadPitch = -0.08f * socialWeight;
        socialTorsoPitch = 0.07f * socialWeight;
        break;
    case SocialGesture::ExcitedPulse:
        socialTorsoPitch = -0.055f * socialWeight;
        break;
    case SocialGesture::ConversationalIdle:
        socialTorsoRoll = 0.018f * socialWeight;
        break;
    case SocialGesture::GreetingWave:
    case SocialGesture::None:
        break;
    }
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

    const bool playerKnockedDown = rideBail || (scene.combatActive &&
        scene.playerCombatState == hakui::combat::CombatState::KnockedDown);
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
    const auto ridePoint = [](
        const hakui::RideAttachmentRig& rig,
        hakui::RideAnchorSemantic semantic,
        float steering = 0.0f,
        float crank = 0.0f
    ) {
        const hakui::RideAnchorPosition point = rig.resolvePosition(
            semantic,
            steering,
            crank
        );
        return Vec3{point.x, point.y, point.z};
    };
    const float bmxSteering = scene.rideable.steeringVisual * 0.48f +
        (scene.rideable.rotationChannel == hakui::RideRotationChannel::BmxSteering
            ? scene.rideable.rideableRotation.y
            : 0.0f);
    const float bmxCrank = player.gaitPhase * 0.42f +
        (scene.rideable.rotationChannel == hakui::RideRotationChannel::BmxCrank
            ? scene.rideable.rideableRotation.x
            : 0.0f);
    Vec3 leftRideHand = ridePoint(
        bmxRig,
        hakui::RideAnchorSemantic::LeftHandGrip,
        bmxSteering
    );
    Vec3 rightRideHand = ridePoint(
        bmxRig,
        hakui::RideAnchorSemantic::RightHandGrip,
        bmxSteering
    );
    const bool regularStance = scene.rideable.body.skateStance ==
        hakui::SkateStance::Regular;
    Vec3 leftRideFoot = ridingBmx
        ? ridePoint(bmxRig, hakui::RideAnchorSemantic::LeftFootAnchor, 0.0f, bmxCrank)
        : ridePoint(
            skateboardRig,
            regularStance
                ? hakui::RideAnchorSemantic::FrontFootAnchor
                : hakui::RideAnchorSemantic::RearFootAnchor
        );
    Vec3 rightRideFoot = ridingBmx
        ? ridePoint(bmxRig, hakui::RideAnchorSemantic::RightFootAnchor, 0.0f, bmxCrank)
        : ridePoint(
            skateboardRig,
            regularStance
                ? hakui::RideAnchorSemantic::RearFootAnchor
                : hakui::RideAnchorSemantic::FrontFootAnchor
        );
    leftRideFoot.y += scene.rideable.leftFootAnchorError;
    rightRideFoot.y += scene.rideable.rightFootAnchorError;
    if (ridingSkateboard) {
        Vec3& frontFoot = regularStance ? leftRideFoot : rightRideFoot;
        Vec3& rearFoot = regularStance ? rightRideFoot : leftRideFoot;
        frontFoot.y += scene.rideable.body.frontFootLift;
        rearFoot.y += scene.rideable.body.rearLegDrive * 0.10f;
        rearFoot.z -= scene.rideable.body.rearLegDrive * 0.12f;
    }

    // A whole-machine tabletop carries contact targets with it. Component
    // tricks (tailwhip/crankflip) deliberately let the feet separate.
    const auto rotateWholeRidePoint = [&](Vec3 point) {
        if (scene.rideable.rotationChannel != hakui::RideRotationChannel::Rideable) {
            return point;
        }
        const float cosineX = std::cos(scene.rideable.rideableRotation.x);
        const float sineX = std::sin(scene.rideable.rideableRotation.x);
        const float yAfterX = point.y * cosineX - point.z * sineX;
        const float zAfterX = point.y * sineX + point.z * cosineX;
        point.y = yAfterX;
        point.z = zAfterX;
        const float cosineZ = std::cos(scene.rideable.rideableRotation.z);
        const float sineZ = std::sin(scene.rideable.rideableRotation.z);
        const float xAfterZ = point.x * cosineZ - point.y * sineZ;
        const float yAfterZ = point.x * sineZ + point.y * cosineZ;
        point.x = xAfterZ;
        point.y = yAfterZ;
        return point;
    };
    leftRideHand = rotateWholeRidePoint(leftRideHand);
    rightRideHand = rotateWholeRidePoint(rightRideHand);
    leftRideFoot = rotateWholeRidePoint(leftRideFoot);
    rightRideFoot = rotateWholeRidePoint(rightRideFoot);
    const Mat4 avatarRoot = multiply(
        translation({
            player.x,
            player.y + bodyBob + groundContact.visualRootAbovePlayerBase,
            player.z
        }),
        multiply(
            rotationY(player.yaw),
            rotationZ(
                rideBail
                    ? 0.74f + scene.rideable.tumbleRadians
                    : (playerKnockedDown ? 1.32f : 0.0f)
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

    auto orientedLocalBox = [&](const Vec3& position,
                                const Vec3& dimensions,
                                float yaw,
                                Uint32 palette = Shell) {
        drawModel(multiply(
            avatarRoot,
            multiply(
                translation(position),
                multiply(rotationY(yaw), scale(dimensions))
            )
        ), palette);
    };

    const auto rotateYawPoint = [](Vec3 point, float yaw) {
        const float cosine = std::cos(yaw);
        const float sine = std::sin(yaw);
        return Vec3{
            point.x * cosine + point.z * sine,
            point.y,
            -point.x * sine + point.z * cosine
        };
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

    auto contactSegment = [&](const Vec3& from,
                              const Vec3& to,
                              float width,
                              Uint32 palette = Shell) {
        const Vec3 delta{to.x - from.x, to.y - from.y, to.z - from.z};
        const float segmentLength = std::sqrt(
            delta.x * delta.x + delta.y * delta.y + delta.z * delta.z
        );
        if (segmentLength <= 0.0001f) {
            return;
        }
        const Vec3 up{
            delta.x / segmentLength,
            delta.y / segmentLength,
            delta.z / segmentLength
        };
        const Vec3 reference = std::fabs(up.z) < 0.90f
            ? Vec3{0.0f, 0.0f, 1.0f}
            : Vec3{1.0f, 0.0f, 0.0f};
        Vec3 right{
            reference.y * up.z - reference.z * up.y,
            reference.z * up.x - reference.x * up.z,
            reference.x * up.y - reference.y * up.x
        };
        const float rightLength = std::sqrt(
            right.x * right.x + right.y * right.y + right.z * right.z
        );
        right = {
            right.x / rightLength,
            right.y / rightLength,
            right.z / rightLength
        };
        const Vec3 forward{
            up.y * right.z - up.z * right.y,
            up.z * right.x - up.x * right.z,
            up.x * right.y - up.y * right.x
        };
        Mat4 orientation = identity();
        orientation.m[0] = right.x;
        orientation.m[1] = right.y;
        orientation.m[2] = right.z;
        orientation.m[4] = up.x;
        orientation.m[5] = up.y;
        orientation.m[6] = up.z;
        orientation.m[8] = forward.x;
        orientation.m[9] = forward.y;
        orientation.m[10] = forward.z;
        const Vec3 midpoint{
            (from.x + to.x) * 0.5f,
            (from.y + to.y) * 0.5f,
            (from.z + to.z) * 0.5f
        };
        drawModel(
            multiply(
                avatarRoot,
                multiply(
                    translation(midpoint),
                    multiply(orientation, scale({width, segmentLength, width}))
                )
            ),
            palette
        );
    };

    auto contactLeg = [&](float side,
                          Vec3 target,
                          float kneeFlex,
                          float hipYaw,
                          float footYaw) {
        target.y -= groundContact.visualRootAbovePlayerBase;
        const Vec3 hip = rotateYawPoint(
            {side * 0.23f, 1.17f - kneeFlex * 0.08f, 0.0f},
            hipYaw
        );
        const Vec3 knee{
            (hip.x + target.x) * 0.5f + side * 0.04f,
            (hip.y + target.y) * 0.5f + 0.10f - kneeFlex * 0.10f,
            (hip.z + target.z) * 0.5f - 0.14f - kneeFlex * 0.12f
        };
        contactSegment(hip, knee, 0.27f);
        contactSegment(knee, target, 0.25f);
        orientedLocalBox(
            {target.x, target.y + 0.06f, target.z + 0.10f},
            {0.32f, 0.14f, 0.50f},
            footYaw,
            Cyan
        );
    };

    auto contactArm = [&](float side,
                          Vec3 target,
                          float shoulderYaw,
                          float elbowFlex) {
        target.y -= groundContact.visualRootAbovePlayerBase;
        const Vec3 shoulder = rotateYawPoint(
            {side * 0.60f, 2.12f, 0.0f},
            shoulderYaw
        );
        const Vec3 elbow{
            (shoulder.x + target.x) * 0.5f + side * 0.12f,
            (shoulder.y + target.y) * 0.5f - 0.04f - elbowFlex * 0.06f,
            (shoulder.z + target.z) * 0.5f - 0.08f - elbowFlex * 0.08f
        };
        contactSegment(shoulder, elbow, 0.23f);
        contactSegment(elbow, target, 0.21f);
        localBox(target, {0.24f, 0.20f, 0.24f}, Midnight);
    };

    if (seated) {
        contactLeg(-1.0f, {-0.34f, 0.34f, 0.58f}, 0.98f, 0.0f, 0.0f);
        contactLeg(1.0f, {0.34f, 0.34f, 0.58f}, 0.98f, 0.0f, 0.0f);
    } else if (mounted && ridingSkateboard) {
        contactLeg(
            -1.0f,
            leftRideFoot,
            scene.rideable.body.leftKneeFlex,
            scene.rideable.body.pelvisYawRelativeToBoard,
            scene.rideable.body.pelvisYawRelativeToBoard
        );
        contactLeg(
            1.0f,
            rightRideFoot,
            scene.rideable.body.rightKneeFlex,
            scene.rideable.body.pelvisYawRelativeToBoard,
            scene.rideable.body.pelvisYawRelativeToBoard
        );
    } else if (mounted && ridingBmx) {
        contactLeg(
            -1.0f,
            leftRideFoot,
            scene.rideable.body.leftKneeFlex,
            scene.rideable.body.pelvisYawRelativeToBoard,
            0.0f
        );
        contactLeg(
            1.0f,
            rightRideFoot,
            scene.rideable.body.rightKneeFlex,
            scene.rideable.body.pelvisYawRelativeToBoard,
            0.0f
        );
    } else if (scene.combatActive) {
        const float stanceStep = std::sin(player.gaitPhase) *
            0.18f * scene.playerStanceBlend;
        leg(-1.0f, -0.10f + stanceStep, 0.22f);
        leg(1.0f, 0.14f - stanceStep, 0.28f);
    } else {
        leg(-1.0f, gait * stride, std::max(0.0f, -gait) * 0.58f);
        leg(1.0f, counterGait * stride, std::max(0.0f, -counterGait) * 0.58f);
    }
    orientedLocalBox(
        {0.0f, 1.20f - scene.rideable.body.landingCompression * 0.12f, 0.0f},
        {0.74f, 0.30f, 0.44f},
        mounted ? scene.rideable.body.pelvisYawRelativeToBoard : 0.0f,
        Midnight
    );

    const bool attackRelease = scene.combatActive &&
        scene.playerCombatState == hakui::combat::CombatState::Release;
    const float attackCommitment = attackRelease
        ? (scene.playerAttack == hakui::combat::AttackSemantic::Cross ? 0.28f : 0.18f)
        : 0.0f;
    const float impactLean = scene.playerCombatState ==
        hakui::combat::CombatState::Staggered ? -0.22f : 0.0f;
    const float rideCompression = mounted
        ? scene.rideable.body.preloadPoseWeight * 0.20f +
            scene.rideable.body.landingCompression * 0.24f
        : 0.0f;
    const float bodyAssistVisual = mounted
        ? scene.rideable.bodyRotationAssist * std::sin(
            std::clamp(scene.rideable.rotationCompletion, 0.0f, 1.0f) * kPi
        )
        : 0.0f;
    const Mat4 torso = multiply(
        avatarRoot,
        multiply(
            translation({
                0.0f,
                1.72f + idleBreath - rideCompression,
                attackCommitment
            }),
            multiply(
                rotationY(
                    mounted ? scene.rideable.body.torsoYawRelativeToBoard : 0.0f
                ),
                multiply(
                    rotationX(
                        airborneLean + expressiveRideLean +
                        (mounted ? scene.rideable.body.torsoLean : 0.0f) +
                        socialTorsoPitch
                    ),
                    multiply(
                        rotationZ(
                            bodySway + expressiveRideSway + impactLean +
                            bodyAssistVisual + socialTorsoRoll
                        ),
                        scale({0.92f, 0.94f, 0.48f})
                    )
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
    if (socialAllowed &&
        scene.socialGesture == SocialGesture::GreetingWave) {
        rightArmAngle = -1.05f - 0.42f * socialWeight;
    }
    if (mounted && ridingBmx) {
        contactArm(
            -1.0f,
            leftRideHand,
            scene.rideable.body.torsoYawRelativeToBoard,
            scene.rideable.body.leftElbowFlex
        );
        contactArm(
            1.0f,
            rightRideHand,
            scene.rideable.body.torsoYawRelativeToBoard,
            scene.rideable.body.rightElbowFlex
        );
    } else if (mounted && ridingSkateboard) {
        const float counterbalance = scene.rideable.body.armCounterbalance;
        Vec3 leftSkateHand = rotateYawPoint(
            {-0.82f, 1.48f + counterbalance * 0.10f,
             -0.10f - counterbalance * 0.24f},
            scene.rideable.body.torsoYawRelativeToBoard
        );
        Vec3 rightSkateHand = rotateYawPoint(
            {0.82f, 1.52f - counterbalance * 0.10f,
             0.12f + counterbalance * 0.24f},
            scene.rideable.body.torsoYawRelativeToBoard
        );
        if (scene.rideable.body.airPose == hakui::RideAirPose::BoardGrab) {
            if (regularStance) {
                rightSkateHand = rightRideFoot;
                rightSkateHand.y += 0.16f;
            } else {
                leftSkateHand = leftRideFoot;
                leftSkateHand.y += 0.16f;
            }
        }
        contactArm(
            -1.0f,
            leftSkateHand,
            scene.rideable.body.torsoYawRelativeToBoard,
            scene.rideable.body.leftElbowFlex
        );
        contactArm(
            1.0f,
            rightSkateHand,
            scene.rideable.body.torsoYawRelativeToBoard,
            scene.rideable.body.rightElbowFlex
        );
    } else {
        arm(-1.0f, leftArmAngle);
        arm(1.0f, rightArmAngle);
    }
    const float headYaw = mounted
        ? scene.rideable.body.headYawRelativeToBoard
        : 0.0f;
    const auto socialHeadBox = [&](const Vec3& position,
                                   const Vec3& dimensions,
                                   Uint32 palette) {
        drawModel(
            multiply(
                avatarRoot,
                multiply(
                    translation(position),
                    multiply(
                        rotationY(headYaw),
                        multiply(
                            rotationX(socialHeadPitch),
                            multiply(rotationZ(socialHeadRoll), scale(dimensions))
                        )
                    )
                )
            ),
            palette
        );
    };
    socialHeadBox(
        {0.0f, 2.28f + idleBreath - rideCompression, 0.0f},
        {0.22f, 0.18f, 0.22f},
        Cyan
    );
    socialHeadBox(
        {0.0f, 2.60f + idleBreath - rideCompression, 0.0f},
        {0.56f, 0.58f, 0.52f},
        Shell
    );

    if (scene.chatBubbleActive && !scene.chatBubbleText.empty()) {
        const float anchorY = player.y +
            groundContact.visualRootAbovePlayerBase + 2.95f + idleBreath -
            rideCompression;
        const float dx = cameraEyeX_ - player.x;
        const float dy = cameraEyeY_ - anchorY;
        const float dz = cameraEyeZ_ - player.z;
        const float cameraDistance = std::sqrt(dx * dx + dy * dy + dz * dz);
        const hakui::social::BubblePresentationLayout layout =
            hakui::social::ChatBubblePresentation::resolve(
                scene.chatBubbleText,
                scene.chatBubbleRemaining,
                scene.chatBubbleTotal,
                cameraDistance,
                scene.chatBubbleStyle.material
            );
        if (layout.visible) {
            const float bubbleYaw = std::atan2(dx, dz);
            const float visualAnchorY = anchorY + layout.verticalOffset;
            const Mat4 bubbleRoot = multiply(
                translation({player.x, visualAnchorY, player.z}),
                rotationY(bubbleYaw)
            );
            const float presentationScale = layout.scale;
            const float panelWidth = layout.width * presentationScale;
            const float panelHeight = layout.height * presentationScale;
            const float tailSize = scene.chatBubbleStyle.material.tailSize *
                presentationScale;
            const float cornerRadius = std::min(
                scene.chatBubbleStyle.material.cornerRadius * presentationScale,
                panelHeight * 0.32f
            );
            const float bodyCenterY = tailSize + panelHeight * 0.5f;

            const auto roundedSurface = [&](float surfaceWidth,
                                            float surfaceHeight,
                                            float radius,
                                            float z,
                                            Uint32 palette) {
                const float middleHeight = std::max(
                    0.02f, surfaceHeight - radius * 2.0f
                );
                drawModel(
                    multiply(
                        bubbleRoot,
                        multiply(
                            translation({0.0f, bodyCenterY, z}),
                            scale({surfaceWidth, middleHeight, 0.042f})
                        )
                    ),
                    palette
                );
                for (const float side : {-1.0f, 1.0f}) {
                    drawModel(
                        multiply(
                            bubbleRoot,
                            multiply(
                                translation({
                                    0.0f,
                                    bodyCenterY + side *
                                        (surfaceHeight * 0.5f - radius * 0.68f),
                                    z
                                }),
                                scale({surfaceWidth - radius * 0.52f,
                                       radius * 0.72f,
                                       0.042f})
                            )
                        ),
                        palette
                    );
                    drawModel(
                        multiply(
                            bubbleRoot,
                            multiply(
                                translation({
                                    0.0f,
                                    bodyCenterY + side *
                                        (surfaceHeight * 0.5f - radius * 0.18f),
                                    z
                                }),
                                scale({surfaceWidth - radius * 1.48f,
                                       radius * 0.36f,
                                       0.042f})
                            )
                        ),
                        palette
                    );
                }
            };
            const auto taperedTail = [&](float size,
                                         float z,
                                         Uint32 palette) {
                for (int segment = 0; segment < 3; ++segment) {
                    const float fraction = static_cast<float>(segment + 1) / 3.0f;
                    const float segmentSize = size * (0.28f + fraction * 0.42f);
                    drawModel(
                        multiply(
                            bubbleRoot,
                            multiply(
                                translation({
                                    0.0f,
                                    size * (0.16f + fraction * 0.56f),
                                    z
                                }),
                                multiply(
                                    rotationZ(kPi * 0.25f),
                                    scale({segmentSize, segmentSize, 0.045f})
                                )
                            )
                        ),
                        palette
                    );
                }
            };

            const Uint32 bubbleAccent = scene.speechIntent ==
                hakui::social::SpeechIntent::Excited ? Magenta : Cyan;
            bindGlass(
                layout.alpha * scene.chatBubbleStyle.material.borderAlpha * 0.30f
            );
            roundedSurface(
                panelWidth + 0.055f,
                panelHeight + 0.055f,
                cornerRadius + 0.025f,
                0.052f,
                bubbleAccent
            );
            taperedTail(tailSize + 0.025f, 0.052f, bubbleAccent);

            bindGlass(
                layout.alpha * scene.chatBubbleStyle.material.backgroundAlpha
            );
            roundedSurface(
                panelWidth,
                panelHeight,
                cornerRadius,
                0.045f,
                Midnight
            );
            taperedTail(tailSize, 0.045f, Midnight);

            std::string wrappedText;
            for (std::size_t line = 0; line < layout.lines.size(); ++line) {
                if (line > 0) wrappedText.push_back('\n');
                wrappedText += fontDisplayText(layout.lines[line], 96);
            }
            const float cellWidth = scene.chatBubbleStyle.material.textScale *
                presentationScale;
            const float cellHeight = cellWidth * 1.18f;
            const float textLeft = -panelWidth * 0.5f +
                scene.chatBubbleStyle.material.padding * presentationScale;
            const float textTop = tailSize + panelHeight -
                scene.chatBubbleStyle.material.padding * presentationScale;
            bindGlass(layout.alpha);
            drawWorldText(
                wrappedText,
                multiply(
                    bubbleRoot,
                    translation({textLeft, textTop, -0.006f})
                ),
                cellWidth,
                cellHeight,
                0.040f,
                96,
                Shell
            );
            bindOpaque();

            const Vec3 bubbleCenter{
                player.x,
                visualAnchorY + tailSize + panelHeight * 0.5f,
                player.z
            };
            const float clipX = viewProjection.m[0] * bubbleCenter.x +
                viewProjection.m[4] * bubbleCenter.y +
                viewProjection.m[8] * bubbleCenter.z + viewProjection.m[12];
            const float clipY = viewProjection.m[1] * bubbleCenter.x +
                viewProjection.m[5] * bubbleCenter.y +
                viewProjection.m[9] * bubbleCenter.z + viewProjection.m[13];
            const float clipW = viewProjection.m[3] * bubbleCenter.x +
                viewProjection.m[7] * bubbleCenter.y +
                viewProjection.m[11] * bubbleCenter.z + viewProjection.m[15];
            bubbleVisualTelemetry_.active = true;
            bubbleVisualTelemetry_.worldX = bubbleCenter.x;
            bubbleVisualTelemetry_.worldY = bubbleCenter.y;
            bubbleVisualTelemetry_.worldZ = bubbleCenter.z;
            if (std::fabs(clipW) > 0.0001f) {
                bubbleVisualTelemetry_.screenX =
                    (clipX / clipW * 0.5f + 0.5f) * static_cast<float>(width);
                bubbleVisualTelemetry_.screenY =
                    (0.5f - clipY / clipW * 0.5f) * static_cast<float>(height);
            }
            bubbleVisualTelemetry_.scale = presentationScale;
            bubbleVisualTelemetry_.alpha = layout.alpha *
                scene.chatBubbleStyle.material.backgroundAlpha;
            bubbleVisualTelemetry_.width = panelWidth;
            bubbleVisualTelemetry_.height = panelHeight + tailSize;
            bubbleVisualTelemetry_.lineCount = layout.lines.size();
            bubbleVisualTelemetry_.styleProfile =
                scene.chatBubbleStyle.profile;
            bubbleVisualTelemetry_.lifePhase = layout.phase;
            bubbleVisualTelemetry_.distanceToCamera = cameraDistance;
            bubbleVisualTelemetry_.anchorError = std::fabs(layout.verticalOffset);
        }
    }

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

    if (scene.chatInputActive) {
        std::string entry = "say  ";
        entry += fontDisplayText(scene.chatInputBuffer, 34);
        entry += "_";
        float entryUnits = 0.0f;
        for (const char character : entry) {
            entryUnits += hakui::social::ChatBubblePresentation::
                glyphAdvanceUnits(character);
        }
        const float panelWidth = std::clamp(
            entryUnits * 0.0044f + 0.18f, 0.54f, 1.28f
        );
        constexpr float panelHeight = 0.13f;
        bindGlass(0.26f);
        drawClipModel(
            multiply(
                translation({0.0f, -0.86f, 0.016f}),
                scale({panelWidth, panelHeight * 0.66f, 0.004f})
            ),
            Cyan
        );
        bindGlass(0.48f);
        drawClipModel(
            multiply(
                translation({0.0f, -0.86f, 0.012f}),
                scale({panelWidth - 0.018f, panelHeight * 0.56f, 0.004f})
            ),
            Midnight
        );
        bindGlass(0.94f);
        drawClipText(
            entry,
            -panelWidth * 0.5f + 0.075f,
            -0.825f,
            0.0044f,
            0.014f,
            Shell
        );
        bindOpaque();
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

    if (glassPipeline_) {
        SDL_ReleaseGPUGraphicsPipeline(device_, glassPipeline_);
        glassPipeline_ = nullptr;
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
