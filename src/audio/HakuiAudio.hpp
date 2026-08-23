#pragma once

#include <SDL3/SDL.h>

enum class HakuiAudioCue {
    FootstepSoft,
    FootstepHard,
    Jump,
    Land,
    Interact,
    Casino,
    VoidRespawn,
    CombatSwing,
    CombatHit,
    CombatGuard,
    Knockdown,
    Recovery,
    RidePop,
    BoardRotation,
    BmxTrick,
    GrindScrape,
    CleanLanding,
    SketchyLanding,
    BailImpact
};

class HakuiAudio {
public:
    bool init();
    void play(HakuiAudioCue cue);
    void adjustVolume(float delta) noexcept;
    float volume() const noexcept;
    void shutdown();

private:
    SDL_AudioStream* stream_ = nullptr;
    float volume_ = 0.55f;
};
