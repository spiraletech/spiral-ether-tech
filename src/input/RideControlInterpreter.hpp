#pragma once

#include <cstdint>
#include <string_view>

#include "input/HakuiInput.hpp"

namespace hakui::input {

enum class FlickDirection : std::uint8_t {
    None,
    Left,
    Right,
    Up,
    Down,
    UpLeft,
    UpRight,
    DownLeft,
    DownRight
};

enum class RightStickOwner : std::uint8_t {
    Camera,
    TrickWindow
};

struct RideControlTuning {
    float cameraDeadzone = 0.16f;
    float preloadSaturationTime = 0.42f;
    float trickWindowDelay = 0.025f;
    float trickWindowDuration = 0.70f;
    float flickDeadzone = 0.22f;
    float flickThreshold = 0.68f;
    float flickReleaseThreshold = 0.30f;
};

struct TrickVector {
    FlickDirection direction = FlickDirection::None;
    float rawX = 0.0f;
    float rawY = 0.0f;
    float normalizedX = 0.0f;
    float normalizedY = 0.0f;
    float magnitude = 0.0f;
    float startTime = 0.0f;
    float releaseTime = 0.0f;
    float gestureDuration = 0.0f;
    bool valid = false;
};

struct RideControlFrame {
    bool rideActive = false;
    bool airborne = false;
    bool popIntent = false;
    bool popPreparing = false;
    float popPreload = 0.0f;
    bool trickWindowArmed = false;
    bool trickWindowListening = false;
    bool trickIntent = false;
    bool grindIntent = false;
    bool balanceIntent = false;
    bool styleIntent = false;
    bool cancelIntent = false;
    bool spinLeftIntent = false;
    bool spinRightIntent = false;
    float propulsionIntent = 0.0f;
    float trickWindowRemaining = 0.0f;
    float rawRightStickX = 0.0f;
    float rawRightStickY = 0.0f;
    float normalizedFlickX = 0.0f;
    float normalizedFlickY = 0.0f;
    RightStickOwner rightStickOwner = RightStickOwner::Camera;
    TrickVector trick{};
};

// GPU-independent PRELOAD -> POP -> AIR -> FLICK recognizer. A tap emits a
// normal pop. A short hold accumulates capped preload and emits the same
// semantic pop on release. Gameplay explicitly arms the trick window only
// after that pop successfully makes the rideable airborne.
class RideControlInterpreter {
public:
    explicit RideControlInterpreter(RideControlTuning tuning = {});

    RideControlFrame update(
        const InputFrame& frame,
        bool rideActive,
        bool airborne,
        float deltaSeconds
    ) noexcept;
    void armTrickWindow() noexcept;
    void closeTrickWindow() noexcept;
    void reset() noexcept;

    const RideControlFrame& diagnostics() const noexcept;
    const RideControlTuning& tuning() const noexcept;

    static FlickDirection classifyDirection(float x, float y) noexcept;
    static std::string_view directionName(FlickDirection direction) noexcept;
    static std::string_view ownerName(RightStickOwner owner) noexcept;
    static void radialDeadzone(
        float x,
        float y,
        float deadzone,
        float& outputX,
        float& outputY
    ) noexcept;

private:
    void completeGesture(RideControlFrame& output) noexcept;
    void clearWindowState() noexcept;

    RideControlTuning tuning_{};
    RideControlFrame diagnostics_{};
    TrickVector lastResolvedTrick_{};
    float clock_ = 0.0f;
    float windowElapsed_ = 0.0f;
    float flickStart_ = 0.0f;
    float peakX_ = 0.0f;
    float peakY_ = 0.0f;
    float peakMagnitude_ = 0.0f;
    bool trickWindowArmed_ = false;
    bool thresholdCrossed_ = false;
    bool popPreparing_ = false;
    float popPreloadSeconds_ = 0.0f;
};

} // namespace hakui::input
