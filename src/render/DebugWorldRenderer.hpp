#pragma once

#include <span>
#include <string_view>

#include <SDL3/SDL.h>

#include "camera/ThirdPersonCameraRig.hpp"
#include "combat/CombatSimulation.hpp"
#include "player/PlayerState.hpp"
#include "player/RideableMovementController.hpp"
#include "social/ChatSystem.hpp"
#include "world/WorldGeometry.hpp"

enum class CameraRole {
    GameplayFollow,
    InteractionFrame,
    CombatFrame,
    TargetFrame,
    DuelFrame,
    Spectator,
    Director
};

enum class InteractionFrame {
    None,
    FusionTable,
    LoungeCouch
};

struct HakuiSceneState {
    bool paused = false;
    bool terminalPowered = false;
    bool cardSuiteActive = false;
    int diceTotal = 0;
    bool combatActive = false;
    bool sparDummyVisible = false;
    hakui::combat::CombatState playerCombatState =
        hakui::combat::CombatState::Inactive;
    hakui::combat::CombatState opponentCombatState =
        hakui::combat::CombatState::Inactive;
    hakui::combat::AttackSemantic playerAttack =
        hakui::combat::AttackSemantic::None;
    hakui::combat::AttackSemantic opponentAttack =
        hakui::combat::AttackSemantic::None;
    float opponentX = 0.0f;
    float opponentY = 0.0f;
    float opponentZ = 0.0f;
    float opponentYaw = 0.0f;
    float playerHitPulse = 0.0f;
    float opponentHitPulse = 0.0f;
    float playerStanceBlend = 0.0f;
    float opponentStanceBlend = 0.0f;
    bool chatInputActive = false;
    std::string_view chatInputBuffer{};
    bool chatBubbleActive = false;
    std::string_view chatBubbleText{};
    float chatBubbleRemaining = 0.0f;
    float chatBubbleTotal = 0.0f;
    hakui::social::SpeechIntent speechIntent =
        hakui::social::SpeechIntent::Neutral;
    hakui::social::SocialGesture socialGesture =
        hakui::social::SocialGesture::None;
    float socialGestureWeight = 0.0f;
    hakui::RideableState rideable{};
};

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
        const PlayerState& player,
        const HakuiSceneState& scene,
        std::span<const hakui::WorldPrimitive> worldGeometry
    );
    void updateCamera(float deltaSeconds, const PlayerState& player);
    void orbitCamera(float horizontalPixels, float verticalPixels);
    void zoomCamera(float wheelSteps);
    void resetCamera();
    void toggleShoulder();
    void adjustLookSensitivity(float delta);
    float lookSensitivity() const noexcept;
    float cameraYaw() const noexcept;
    float cameraPitch() const noexcept;
    float cameraDistance() const noexcept;
    float cameraShoulderSide() const noexcept;
    float cameraWorldX() const noexcept;
    float cameraWorldY() const noexcept;
    float cameraWorldZ() const noexcept;
    float cameraTargetX() const noexcept;
    float cameraTargetY() const noexcept;
    float cameraTargetZ() const noexcept;
    float fieldOfViewDegrees() const noexcept;
    void setCameraRole(CameraRole role);
    void frameInteraction(InteractionFrame frame);
    void setCombatTarget(float x, float y, float z) noexcept;
    CameraRole cameraRole() const noexcept;
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
    float cameraTargetX_ = 0.0f;
    float cameraTargetY_ = 1.25f;
    float cameraTargetZ_ = 0.0f;
    float cameraEyeX_ = 0.0f;
    float cameraEyeY_ = 0.0f;
    float cameraEyeZ_ = 0.0f;
    hakui::camera::ThirdPersonCameraRig cameraRig_{};
    CameraRole cameraRole_ = CameraRole::GameplayFollow;
    InteractionFrame interactionFrame_ = InteractionFrame::None;
    float combatTargetX_ = 0.0f;
    float combatTargetY_ = 1.25f;
    float combatTargetZ_ = 0.0f;
    bool cameraInitialized_ = false;
};
