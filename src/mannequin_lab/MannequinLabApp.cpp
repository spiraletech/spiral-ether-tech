#include "mannequin_lab/MannequinLabApp.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace {

const std::array<hakui::WorldPrimitive, 7> kStudio{{
    {hakui::WorldPrimitiveKind::Floor,
     hakui::MaterialRole::IndustrialDark,
     0.0f, -0.08f, 0.0f, 8.0f, 0.10f, 8.0f},
    {hakui::WorldPrimitiveKind::Wall,
     hakui::MaterialRole::PowderConcrete,
     0.0f, 2.0f, 3.6f, 8.0f, 4.0f, 0.10f},
    {hakui::WorldPrimitiveKind::Platform,
     hakui::MaterialRole::PowderConcrete,
     0.0f, 0.03f, 0.0f, 2.6f, 0.08f, 2.6f},
    {hakui::WorldPrimitiveKind::Signage,
     hakui::MaterialRole::CrtCyan,
     0.0f, 0.012f, 0.0f, 0.035f, 0.02f, 5.0f},
    {hakui::WorldPrimitiveKind::Signage,
     hakui::MaterialRole::SignalMagenta,
     0.0f, 0.013f, 0.0f, 5.0f, 0.02f, 0.035f},
    {hakui::WorldPrimitiveKind::Signage,
     hakui::MaterialRole::SodiumAmber,
     -1.35f, 1.55f, 3.48f, 0.035f, 3.1f, 0.035f},
    {hakui::WorldPrimitiveKind::Signage,
     hakui::MaterialRole::CrtCyan,
     1.35f, 1.55f, 3.48f, 0.035f, 3.1f, 0.035f}
}};

constexpr float kPi = 3.14159265358979323846f;

} // namespace

bool MannequinLabApp::boot()
{
    SDL_Log("[MANNEQUIN LAB] boot // rig science v0.1");
    if (!initPlatform() || !initGPU()) {
        return false;
    }

    mannequin_.displayName = "MANNEQUIN";
    mannequin_.locomotion = LocomotionMode::OnFoot;
    mannequin_.activity = PlayerActivity::Roaming;
    mannequin_.x = 0.0f;
    mannequin_.y = 0.0f;
    mannequin_.z = 0.0f;
    mannequin_.yaw = 0.0f;
    mannequin_.grounded = true;
    mannequin_.idlePhase = 0.0f;

    applyPreset(PosePreset::Neutral);
    renderer_.resetCamera();
    previousCounter_ = SDL_GetPerformanceCounter();
    updateWindowTitle();

    SDL_Log("[MANNEQUIN LAB] 1 neutral // 2 T // 3 A // 4 crouch // 5 ollie pop");
    SDL_Log("[MANNEQUIN LAB] Q/E pelvis // A/D torso yaw // W/S torso lean // [/] knees");
    SDL_Log("[MANNEQUIN LAB] J joints // arrows rotate mannequin // RMB orbit // wheel zoom // R camera // ESC quit");
    return true;
}

bool MannequinLabApp::initPlatform()
{
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "[MANNEQUIN LAB] SDL init failed: %s", SDL_GetError());
        return false;
    }

    window_ = SDL_CreateWindow(
        "HAKUI MANNEQUIN LAB // RIG SCIENCE v0.1",
        1280,
        720,
        SDL_WINDOW_RESIZABLE | SDL_WINDOW_HIGH_PIXEL_DENSITY
    );
    if (!window_) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "[MANNEQUIN LAB] window failed: %s", SDL_GetError());
        return false;
    }
    return true;
}

bool MannequinLabApp::initGPU()
{
    constexpr SDL_GPUShaderFormat formats = static_cast<SDL_GPUShaderFormat>(
        SDL_GPU_SHADERFORMAT_SPIRV |
        SDL_GPU_SHADERFORMAT_DXIL |
        SDL_GPU_SHADERFORMAT_MSL
    );

    gpu_ = SDL_CreateGPUDevice(formats, true, nullptr);
    if (!gpu_) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "[MANNEQUIN LAB] GPU device failed: %s", SDL_GetError());
        return false;
    }
    if (!SDL_ClaimWindowForGPUDevice(gpu_, window_)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "[MANNEQUIN LAB] GPU claim failed: %s", SDL_GetError());
        return false;
    }
    if (!renderer_.init(gpu_, window_)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "[MANNEQUIN LAB] renderer init failed: %s", SDL_GetError());
        return false;
    }
    SDL_Log("[MANNEQUIN LAB] renderer backend: %s", SDL_GetGPUDeviceDriver(gpu_));
    return true;
}

void MannequinLabApp::applyPreset(PosePreset preset)
{
    preset_ = preset;
    pose_ = {};
    pose_.skateStance = hakui::SkateStance::Regular;
    pose_.footContact = hakui::RideFootContactState::Anchored;

    switch (preset_) {
        case PosePreset::Neutral:
            pose_.leftKneeFlex = 0.18f;
            pose_.rightKneeFlex = 0.18f;
            pose_.leftElbowFlex = 0.10f;
            pose_.rightElbowFlex = 0.10f;
            break;
        case PosePreset::TPose:
            pose_.leftKneeFlex = 0.04f;
            pose_.rightKneeFlex = 0.04f;
            pose_.leftElbowFlex = 0.02f;
            pose_.rightElbowFlex = 0.02f;
            break;
        case PosePreset::APose:
            pose_.leftKneeFlex = 0.08f;
            pose_.rightKneeFlex = 0.08f;
            pose_.leftElbowFlex = 0.12f;
            pose_.rightElbowFlex = 0.12f;
            break;
        case PosePreset::Crouch:
            pose_.leftKneeFlex = 0.92f;
            pose_.rightKneeFlex = 0.92f;
            pose_.preloadPoseWeight = 0.74f;
            pose_.torsoLean = 0.18f;
            pose_.leftElbowFlex = 0.28f;
            pose_.rightElbowFlex = 0.28f;
            break;
        case PosePreset::OlliePop:
            pose_.pelvisYawRelativeToBoard = 1.50f;
            pose_.torsoYawRelativeToBoard = 0.58f;
            pose_.headYawRelativeToBoard = 0.10f;
            pose_.leftKneeFlex = 0.72f;
            pose_.rightKneeFlex = 0.32f;
            pose_.frontFootLift = 0.08f;
            pose_.rearLegDrive = 0.56f;
            pose_.torsoLean = 0.08f;
            pose_.armCounterbalance = -0.28f;
            pose_.airPose = hakui::RideAirPose::OlliePop;
            break;
    }
    updateWindowTitle();
}

void MannequinLabApp::adjustKnees(float delta)
{
    pose_.leftKneeFlex = std::clamp(pose_.leftKneeFlex + delta, 0.0f, 1.35f);
    pose_.rightKneeFlex = std::clamp(pose_.rightKneeFlex + delta, 0.0f, 1.35f);
}

void MannequinLabApp::adjustPelvisYaw(float delta)
{
    pose_.pelvisYawRelativeToBoard = std::clamp(
        pose_.pelvisYawRelativeToBoard + delta,
        -kPi * 0.55f,
        kPi * 0.55f
    );
}

void MannequinLabApp::adjustTorsoYaw(float delta)
{
    pose_.torsoYawRelativeToBoard = std::clamp(
        pose_.torsoYawRelativeToBoard + delta,
        -1.30f,
        1.30f
    );
}

void MannequinLabApp::adjustTorsoLean(float delta)
{
    pose_.torsoLean = std::clamp(pose_.torsoLean + delta, -0.55f, 0.65f);
}

std::string_view MannequinLabApp::poseLabel() const noexcept
{
    switch (preset_) {
        case PosePreset::Neutral: return "NEUTRAL";
        case PosePreset::TPose: return "T-POSE";
        case PosePreset::APose: return "A-POSE";
        case PosePreset::Crouch: return "CROUCH";
        case PosePreset::OlliePop: return "OLLIE-POP";
    }
    return "UNKNOWN";
}

void MannequinLabApp::updateWindowTitle()
{
    if (!window_) return;
    char title[512];
    SDL_snprintf(
        title,
        sizeof(title),
        "HAKUI MANNEQUIN LAB v0.1 // %.*s // PELVIS %.2f // TORSO %.2f // LEAN %.2f // KNEES %.2f/%.2f // JOINTS %s",
        static_cast<int>(poseLabel().size()), poseLabel().data(),
        pose_.pelvisYawRelativeToBoard,
        pose_.torsoYawRelativeToBoard,
        pose_.torsoLean,
        pose_.leftKneeFlex,
        pose_.rightKneeFlex,
        showJoints_ ? "ON" : "OFF"
    );
    SDL_SetWindowTitle(window_, title);
}

SDL_AppResult MannequinLabApp::handleEvent(const SDL_Event& event)
{
    if (event.type == SDL_EVENT_QUIT) {
        return SDL_APP_SUCCESS;
    }

    if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN &&
        event.button.button == SDL_BUTTON_RIGHT) {
        cameraDragging_ = true;
        (void)SDL_SetWindowRelativeMouseMode(window_, true);
        return SDL_APP_CONTINUE;
    }
    if (event.type == SDL_EVENT_MOUSE_BUTTON_UP &&
        event.button.button == SDL_BUTTON_RIGHT) {
        cameraDragging_ = false;
        (void)SDL_SetWindowRelativeMouseMode(window_, false);
        return SDL_APP_CONTINUE;
    }
    if (event.type == SDL_EVENT_MOUSE_MOTION && cameraDragging_) {
        renderer_.orbitCamera(event.motion.xrel, event.motion.yrel);
        return SDL_APP_CONTINUE;
    }
    if (event.type == SDL_EVENT_MOUSE_WHEEL) {
        renderer_.zoomCamera(-event.wheel.y);
        return SDL_APP_CONTINUE;
    }

    if (event.type != SDL_EVENT_KEY_DOWN) {
        return SDL_APP_CONTINUE;
    }

    const float repeatScale = event.key.repeat ? 0.45f : 1.0f;
    bool poseChanged = true;
    switch (event.key.scancode) {
        case SDL_SCANCODE_ESCAPE: return SDL_APP_SUCCESS;
        case SDL_SCANCODE_1: applyPreset(PosePreset::Neutral); break;
        case SDL_SCANCODE_2: applyPreset(PosePreset::TPose); break;
        case SDL_SCANCODE_3: applyPreset(PosePreset::APose); break;
        case SDL_SCANCODE_4: applyPreset(PosePreset::Crouch); break;
        case SDL_SCANCODE_5: applyPreset(PosePreset::OlliePop); break;
        case SDL_SCANCODE_Q: adjustPelvisYaw(-0.08f * repeatScale); break;
        case SDL_SCANCODE_E: adjustPelvisYaw(0.08f * repeatScale); break;
        case SDL_SCANCODE_A: adjustTorsoYaw(-0.07f * repeatScale); break;
        case SDL_SCANCODE_D: adjustTorsoYaw(0.07f * repeatScale); break;
        case SDL_SCANCODE_W: adjustTorsoLean(-0.05f * repeatScale); break;
        case SDL_SCANCODE_S: adjustTorsoLean(0.05f * repeatScale); break;
        case SDL_SCANCODE_LEFTBRACKET: adjustKnees(-0.07f * repeatScale); break;
        case SDL_SCANCODE_RIGHTBRACKET: adjustKnees(0.07f * repeatScale); break;
        case SDL_SCANCODE_LEFT:
            mannequin_.yaw -= 0.10f * repeatScale;
            break;
        case SDL_SCANCODE_RIGHT:
            mannequin_.yaw += 0.10f * repeatScale;
            break;
        case SDL_SCANCODE_J:
            if (!event.key.repeat) showJoints_ = !showJoints_;
            break;
        case SDL_SCANCODE_R:
            if (!event.key.repeat) renderer_.resetCamera();
            poseChanged = false;
            break;
        default:
            poseChanged = false;
            break;
    }
    if (poseChanged) updateWindowTitle();
    return SDL_APP_CONTINUE;
}

SDL_AppResult MannequinLabApp::tick()
{
    const Uint64 now = SDL_GetPerformanceCounter();
    const Uint64 frequency = SDL_GetPerformanceFrequency();
    float dt = static_cast<float>(now - previousCounter_) /
               static_cast<float>(frequency);
    previousCounter_ = now;
    dt = std::clamp(dt, 0.0f, 0.10f);

    mannequin_.idlePhase += dt * 0.55f;
    renderer_.updateCamera(dt, mannequin_);
    if (!render()) {
        return SDL_APP_FAILURE;
    }
    return SDL_APP_CONTINUE;
}

bool MannequinLabApp::render()
{
    SDL_GPUCommandBuffer* commands = SDL_AcquireGPUCommandBuffer(gpu_);
    if (!commands) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "[MANNEQUIN LAB] command buffer failed: %s", SDL_GetError());
        return false;
    }

    SDL_GPUTexture* swapchain = nullptr;
    Uint32 width = 0;
    Uint32 height = 0;
    if (!SDL_WaitAndAcquireGPUSwapchainTexture(
            commands, window_, &swapchain, &width, &height)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "[MANNEQUIN LAB] swapchain failed: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(commands);
        return false;
    }
    if (!swapchain) {
        SDL_CancelGPUCommandBuffer(commands);
        return true;
    }

    HakuiSceneState scene;
    scene.mannequinLab = true;
    scene.mannequinShowJoints = showJoints_;
    scene.mannequinPosePreset = static_cast<std::uint8_t>(preset_);
    scene.mannequinPoseLabel = poseLabel();
    scene.rideable.body = pose_;
    scene.rideable.phase = hakui::RidePhase::Grounded;
    scene.localDisplayName = "";
    scene.localHandle = "";

    if (!renderer_.render(
            commands,
            swapchain,
            width,
            height,
            mannequin_,
            scene,
            kStudio)) {
        SDL_LogError(SDL_LOG_CATEGORY_APPLICATION,
                     "[MANNEQUIN LAB] render failed: %s", SDL_GetError());
        SDL_CancelGPUCommandBuffer(commands);
        return false;
    }
    return SDL_SubmitGPUCommandBuffer(commands);
}

void MannequinLabApp::shutdown()
{
    if (window_) {
        (void)SDL_SetWindowRelativeMouseMode(window_, false);
    }
    renderer_.shutdown();
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
