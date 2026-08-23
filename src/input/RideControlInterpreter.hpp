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
    TrickCapture
};

struct RideControlTuning {
    float cameraDeadzone = 0.16f;
    float trickDeadzone = 0.22f;
    float flickActivationThreshold = 0.68f;
    float flickReleaseThreshold = 0.30f;
    float minimumFlickDuration = 0.035f;
    float maximumFlickDuration = 0.55f;
    float maximumTapDuration = 0.24f;
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
    bool activationHeld = false;
    bool captureActive = false;
    bool standardPop = false;
    bool trickResolved = false;
    bool grind = false;
    bool balance = false;
    bool style = false;
    bool cancel = false;
    bool spinLeft = false;
    bool spinRight = false;
    float propulsion = 0.0f;
    float rawRightStickX = 0.0f;
    float rawRightStickY = 0.0f;
    float normalizedTrickX = 0.0f;
    float normalizedTrickY = 0.0f;
    RightStickOwner rightStickOwner = RightStickOwner::Camera;
    TrickVector trick{};
};

// Stateful, GPU-independent ownership and flick recognizer. It consumes only
// semantic actions/axes and never receives SDL button names or events.
class RideControlInterpreter {
public:
    explicit RideControlInterpreter(RideControlTuning tuning = {});

    RideControlFrame update(
        const InputFrame& frame,
        bool rideActive,
        float deltaSeconds
    ) noexcept;
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
    enum class CaptureState : std::uint8_t {
        Idle,
        Capturing,
        ConsumedAwaitRelease
    };

    void beginCapture() noexcept;
    void completeGesture(RideControlFrame& output) noexcept;

    RideControlTuning tuning_{};
    RideControlFrame diagnostics_{};
    CaptureState captureState_ = CaptureState::Idle;
    float clock_ = 0.0f;
    float captureStart_ = 0.0f;
    float peakX_ = 0.0f;
    float peakY_ = 0.0f;
    float peakMagnitude_ = 0.0f;
    bool thresholdCrossed_ = false;
    bool previousActivationHeld_ = false;
    bool requireActivationRelease_ = false;
    TrickVector lastResolvedTrick_{};
};

} // namespace hakui::input
