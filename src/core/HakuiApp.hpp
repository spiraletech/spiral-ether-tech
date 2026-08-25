#pragma once

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>

#include <SDL3/SDL.h>

#include "audio/HakuiAudio.hpp"
#include "avatar/HakuiSkeleton.hpp"
#include "combat/CombatSimulation.hpp"
#include "games/GameTerminal.hpp"
#include "interaction/InteractionService.hpp"
#include "input/SdlInputBridge.hpp"
#include "input/RideControlInterpreter.hpp"
#include "observer/ExpertObserver.hpp"
#include "player/PlayerMovementController.hpp"
#include "player/RideableMovementController.hpp"
#include "player/PlayerState.hpp"
#include "render/DebugWorldRenderer.hpp"
#include "social/ChatSystem.hpp"
#include "spiral/SpiralKernel.hpp"
#include "systems/LocomotionRouter.hpp"
#include "world/WorldState.hpp"
#include "world/BlackRoom.hpp"

class HakuiApp {
public:
    bool boot();
    SDL_AppResult handleEvent(const SDL_Event& event);
    SDL_AppResult tick();
    void shutdown();

private:
    bool initPlatform();
    bool initGPU();
    void initSpiralCore();
    void switchLocomotion(LocomotionMode mode, std::string_view label);
    void requestRideLocomotion(LocomotionMode mode, std::string_view label);
    void showInputStatus(std::string message, float seconds = 2.8f);
    void interactWithTerminal(hakui::InteractionVerb verb);
    void handleCasinoContextAction();
    void leaveCurrentInteraction();
    void handlePrimaryInteraction();
    void toggleCombat();
    void updateCombat(float dt, const hakui::input::DisciplineIntent& intent);
    bool setCameraCapture(bool enabled);
    void setPaused(bool paused);
    void beginChatInput();
    void commitChatInput();
    void cancelChatInput();
    void openGamepad(SDL_JoystickID instanceId);
    void updateHud();
    void update(float dt);
    bool render();
    void recordObserverEvent(std::string_view category, std::string_view message);
    hakui::observer::CaptureContext buildObserverContext() const;
    void captureExpertSnapshot();

    SDL_Window* window_ = nullptr;
    SDL_GPUDevice* gpu_ = nullptr;
    SDL_Gamepad* gamepad_ = nullptr;
    Uint64 previousCounter_ = 0;
    float titleTimer_ = 0.0f;
    float footstepDistance_ = 0.0f;
    bool cameraDragging_ = false;
    int cameraCaptureWarmupFrames_ = 0;
    bool paused_ = false;
    bool quitRequested_ = false;
    bool developerRideFallback_ = false;
    bool alternateFootstep_ = false;
    float inputStatusTimer_ = 0.0f;
    std::string inputStatus_;
    float opponentDecisionTimer_ = 1.15f;
    float playerHitPulse_ = 0.0f;
    float opponentHitPulse_ = 0.0f;
    bool opponentCrossNext_ = false;
    bool expertCaptureRequested_ = false;
    float socialPreviewCaptureDelay_ = -1.0f;
    std::filesystem::path lastObserverBundle_;

    // Spiral is the client's orchestration spine. It remains independent from
    // SDL/rendering and from optional legacy avatar backends.
    spiral::SpiralKernel spiral_;
    spiral::RouterBus::ListenerId spiralListener_ = 0;
    hakui::InteractionService interactions_{spiral_.router()};
    std::shared_ptr<hakui::games::GameTerminal> terminal_;

    HakuiSkeleton avatarSkeleton_;
    HakuiAudio audio_;
    hakui::input::SdlInputBridge inputBridge_;
    hakui::input::InputFrame inputFrame_{};
    hakui::input::RideControlInterpreter rideControls_{};
    hakui::input::RideControlFrame rideControlFrame_{};
    DebugWorldRenderer debugRenderer_;
    WorldState world_;
    hakui::BlackRoom blackRoom_;
    hakui::combat::CombatSimulation combat_;
    hakui::social::ChatSystem chat_;
    PlayerState player_;
    hakui::PlayerMovementController movement_;
    hakui::RideableMovementController rideable_;
    LocomotionRouter locomotion_{player_};
    hakui::observer::RuntimeEventJournal observerJournal_{96};
};
