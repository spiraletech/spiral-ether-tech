#pragma once

#include <memory>
#include <string_view>

#include <SDL3/SDL.h>

#include "audio/HakuiAudio.hpp"
#include "avatar/HakuiSkeleton.hpp"
#include "combat/CombatSimulation.hpp"
#include "games/GameTerminal.hpp"
#include "interaction/InteractionService.hpp"
#include "player/PlayerMovementController.hpp"
#include "player/PlayerState.hpp"
#include "render/DebugWorldRenderer.hpp"
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
    void interactWithTerminal(hakui::InteractionVerb verb);
    void handleCasinoCommand(SDL_Keycode key);
    void handlePrimaryInteraction();
    void toggleCombat();
    void updateCombat(float dt, const bool* keys);
    void setPaused(bool paused);
    void openGamepad(SDL_JoystickID instanceId);
    void handleGamepadButton(SDL_GamepadButton button);
    void updateHud();
    void update(float dt);
    bool render();

    SDL_Window* window_ = nullptr;
    SDL_GPUDevice* gpu_ = nullptr;
    SDL_Gamepad* gamepad_ = nullptr;
    Uint64 previousCounter_ = 0;
    float titleTimer_ = 0.0f;
    float footstepDistance_ = 0.0f;
    bool cameraDragging_ = false;
    bool paused_ = false;
    bool jumpQueued_ = false;
    bool combatRecoverQueued_ = false;
    bool alternateFootstep_ = false;
    hakui::combat::AttackSemantic combatAttackQueued_ =
        hakui::combat::AttackSemantic::None;
    float opponentDecisionTimer_ = 1.15f;
    float playerHitPulse_ = 0.0f;
    float opponentHitPulse_ = 0.0f;
    bool opponentCrossNext_ = false;

    // Spiral is the client's orchestration spine. It remains independent from
    // SDL/rendering and from optional legacy avatar backends.
    spiral::SpiralKernel spiral_;
    spiral::RouterBus::ListenerId spiralListener_ = 0;
    hakui::InteractionService interactions_{spiral_.router()};
    std::shared_ptr<hakui::games::GameTerminal> terminal_;

    HakuiSkeleton avatarSkeleton_;
    HakuiAudio audio_;
    DebugWorldRenderer debugRenderer_;
    WorldState world_;
    hakui::BlackRoom blackRoom_;
    hakui::combat::CombatSimulation combat_;
    PlayerState player_;
    hakui::PlayerMovementController movement_;
    LocomotionRouter locomotion_{player_};
};
