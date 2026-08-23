#include "input/RideControlInterpreter.hpp"

#include <algorithm>
#include <cmath>

namespace hakui::input {

RideControlInterpreter::RideControlInterpreter(RideControlTuning tuning)
    : tuning_(tuning)
{
    tuning_.cameraDeadzone = std::clamp(tuning_.cameraDeadzone, 0.0f, 0.95f);
    tuning_.preloadSaturationTime = std::clamp(
        tuning_.preloadSaturationTime,
        0.08f,
        0.80f
    );
    tuning_.trickWindowDelay = std::clamp(
        tuning_.trickWindowDelay,
        0.0f,
        0.25f
    );
    tuning_.trickWindowDuration = std::clamp(
        tuning_.trickWindowDuration,
        0.10f,
        1.50f
    );
    tuning_.flickDeadzone = std::clamp(tuning_.flickDeadzone, 0.0f, 0.95f);
    tuning_.flickThreshold = std::clamp(
        tuning_.flickThreshold,
        tuning_.flickDeadzone,
        1.0f
    );
    tuning_.flickReleaseThreshold = std::clamp(
        tuning_.flickReleaseThreshold,
        tuning_.flickDeadzone,
        tuning_.flickThreshold
    );
}

RideControlFrame RideControlInterpreter::update(
    const InputFrame& frame,
    bool rideActive,
    bool airborne,
    float deltaSeconds
) noexcept
{
    const float dt = std::clamp(
        std::isfinite(deltaSeconds) ? deltaSeconds : 0.0f,
        0.0f,
        0.10f
    );
    clock_ += dt;

    RideControlFrame output;
    output.rideActive = rideActive;
    output.airborne = rideActive && airborne;
    const ActionState& jump = frame.action(Action::Jump);
    if (rideActive && !airborne) {
        if (jump.pressed && !jump.held) {
            // Some input backends can report an entire tap in one sample.
            output.popIntent = true;
            output.popPreload = 0.0f;
            popPreparing_ = false;
            popPreloadSeconds_ = 0.0f;
        } else if (jump.pressed && jump.held && !popPreparing_) {
            popPreparing_ = true;
            popPreloadSeconds_ = 0.0f;
        }
        if (popPreparing_ && jump.held) {
            popPreloadSeconds_ = std::min(
                tuning_.preloadSaturationTime,
                popPreloadSeconds_ + dt
            );
        } else if (popPreparing_ && !jump.held) {
            output.popIntent = true;
            output.popPreload = std::clamp(
                popPreloadSeconds_ / tuning_.preloadSaturationTime,
                0.0f,
                1.0f
            );
            popPreparing_ = false;
            popPreloadSeconds_ = 0.0f;
        }
    }
    output.popPreparing = popPreparing_;
    if (!output.popIntent) {
        output.popPreload = popPreparing_
            ? std::clamp(
                popPreloadSeconds_ / tuning_.preloadSaturationTime,
                0.0f,
                1.0f
            )
            : 0.0f;
    }
    output.grindIntent = rideActive && frame.action(Action::Grind).held;
    output.balanceIntent = rideActive && frame.action(Action::Balance).held;
    output.styleIntent = rideActive && frame.action(Action::PrimaryAction).pressed;
    output.cancelIntent = rideActive && frame.action(Action::Cancel).pressed;
    output.spinLeftIntent = rideActive && frame.action(Action::SpinLeft).held;
    output.spinRightIntent = rideActive && frame.action(Action::SpinRight).held;
    output.propulsionIntent = rideActive
        ? std::clamp(
            std::max(
                frame.axis(Axis::Accelerate),
                frame.action(Action::Accelerate).value
            ),
            0.0f,
            1.0f
        )
        : 0.0f;
    output.rawRightStickX = std::clamp(
        frame.axis(Axis::RightStickX),
        -1.0f,
        1.0f
    );
    output.rawRightStickY = std::clamp(
        frame.axis(Axis::RightStickY),
        -1.0f,
        1.0f
    );
    radialDeadzone(
        output.rawRightStickX,
        output.rawRightStickY,
        tuning_.flickDeadzone,
        output.normalizedFlickX,
        output.normalizedFlickY
    );
    output.trick = lastResolvedTrick_;

    if (!rideActive || frame.gamepadDisconnected) {
        reset();
        output.rideActive = rideActive;
        output.airborne = false;
        diagnostics_ = output;
        return output;
    }

    if (trickWindowArmed_ && !airborne) {
        clearWindowState();
    }
    if (!trickWindowArmed_) {
        diagnostics_ = output;
        return output;
    }

    windowElapsed_ += dt;
    const float totalWindow =
        tuning_.trickWindowDelay + tuning_.trickWindowDuration;
    output.trickWindowArmed = true;
    output.trickWindowRemaining = std::max(
        0.0f,
        totalWindow - windowElapsed_
    );

    output.rightStickOwner = RightStickOwner::TrickWindow;
    const float magnitude = std::min(
        1.0f,
        std::sqrt(
            output.rawRightStickX * output.rawRightStickX +
            output.rawRightStickY * output.rawRightStickY
        )
    );
    if (magnitude > peakMagnitude_) {
        peakMagnitude_ = magnitude;
        peakX_ = output.rawRightStickX;
        peakY_ = output.rawRightStickY;
    }
    if (!thresholdCrossed_ && magnitude >= tuning_.flickThreshold) {
        thresholdCrossed_ = true;
        flickStart_ = clock_;
    }

    if (windowElapsed_ < tuning_.trickWindowDelay) {
        diagnostics_ = output;
        return output;
    }

    output.trickWindowListening = true;

    const bool flickReleased = thresholdCrossed_ &&
        magnitude <= tuning_.flickReleaseThreshold;
    const bool timedOut = windowElapsed_ >= totalWindow;
    if (flickReleased || timedOut) {
        if (thresholdCrossed_) {
            completeGesture(output);
        }
        clearWindowState();
        output.trickWindowArmed = false;
        output.trickWindowListening = false;
        output.trickWindowRemaining = 0.0f;
        output.rightStickOwner = RightStickOwner::Camera;
    }

    diagnostics_ = output;
    return output;
}

void RideControlInterpreter::armTrickWindow() noexcept
{
    clearWindowState();
    trickWindowArmed_ = true;
    lastResolvedTrick_ = {};
    diagnostics_.trick = {};
    diagnostics_.trickWindowArmed = true;
    diagnostics_.trickWindowListening = false;
    diagnostics_.trickWindowRemaining =
        tuning_.trickWindowDelay + tuning_.trickWindowDuration;
    diagnostics_.rightStickOwner = RightStickOwner::Camera;
}

void RideControlInterpreter::closeTrickWindow() noexcept
{
    clearWindowState();
    diagnostics_.trickWindowArmed = false;
    diagnostics_.trickWindowListening = false;
    diagnostics_.trickWindowRemaining = 0.0f;
    diagnostics_.rightStickOwner = RightStickOwner::Camera;
}

void RideControlInterpreter::reset() noexcept
{
    diagnostics_ = {};
    lastResolvedTrick_ = {};
    clearWindowState();
    popPreparing_ = false;
    popPreloadSeconds_ = 0.0f;
}

const RideControlFrame& RideControlInterpreter::diagnostics() const noexcept
{
    return diagnostics_;
}

const RideControlTuning& RideControlInterpreter::tuning() const noexcept
{
    return tuning_;
}

FlickDirection RideControlInterpreter::classifyDirection(float x, float y) noexcept
{
    if (!std::isfinite(x) || !std::isfinite(y) ||
        std::sqrt(x * x + y * y) < 0.001f) {
        return FlickDirection::None;
    }
    constexpr float diagonalBoundary = 0.41421356237f;
    const float absoluteX = std::fabs(x);
    const float absoluteY = std::fabs(y);
    if (absoluteY <= absoluteX * diagonalBoundary) {
        return x < 0.0f ? FlickDirection::Left : FlickDirection::Right;
    }
    if (absoluteX <= absoluteY * diagonalBoundary) {
        return y < 0.0f ? FlickDirection::Up : FlickDirection::Down;
    }
    if (y < 0.0f) {
        return x < 0.0f ? FlickDirection::UpLeft : FlickDirection::UpRight;
    }
    return x < 0.0f ? FlickDirection::DownLeft : FlickDirection::DownRight;
}

std::string_view RideControlInterpreter::directionName(
    FlickDirection direction
) noexcept
{
    switch (direction) {
        case FlickDirection::None: return "NONE";
        case FlickDirection::Left: return "LEFT";
        case FlickDirection::Right: return "RIGHT";
        case FlickDirection::Up: return "UP";
        case FlickDirection::Down: return "DOWN";
        case FlickDirection::UpLeft: return "UP_LEFT";
        case FlickDirection::UpRight: return "UP_RIGHT";
        case FlickDirection::DownLeft: return "DOWN_LEFT";
        case FlickDirection::DownRight: return "DOWN_RIGHT";
    }
    return "NONE";
}

std::string_view RideControlInterpreter::ownerName(RightStickOwner owner) noexcept
{
    return owner == RightStickOwner::TrickWindow ? "TRICK_WINDOW" : "CAMERA";
}

void RideControlInterpreter::radialDeadzone(
    float x,
    float y,
    float deadzone,
    float& outputX,
    float& outputY
) noexcept
{
    outputX = 0.0f;
    outputY = 0.0f;
    if (!std::isfinite(x) || !std::isfinite(y)) {
        return;
    }
    const float safeDeadzone = std::clamp(deadzone, 0.0f, 0.95f);
    const float magnitude = std::min(std::sqrt(x * x + y * y), 1.0f);
    if (magnitude <= safeDeadzone || magnitude <= 0.0001f) {
        return;
    }
    const float normalizedMagnitude =
        (magnitude - safeDeadzone) / (1.0f - safeDeadzone);
    outputX = x / magnitude * normalizedMagnitude;
    outputY = y / magnitude * normalizedMagnitude;
}

void RideControlInterpreter::completeGesture(RideControlFrame& output) noexcept
{
    TrickVector trick;
    trick.direction = classifyDirection(peakX_, peakY_);
    trick.rawX = peakX_;
    trick.rawY = peakY_;
    trick.magnitude = std::clamp(peakMagnitude_, 0.0f, 1.0f);
    radialDeadzone(
        peakX_,
        peakY_,
        tuning_.flickDeadzone,
        trick.normalizedX,
        trick.normalizedY
    );
    trick.startTime = flickStart_;
    trick.releaseTime = clock_;
    trick.gestureDuration = std::max(0.0f, clock_ - flickStart_);
    trick.valid = trick.direction != FlickDirection::None;
    output.trick = trick;
    output.trickIntent = trick.valid;
    lastResolvedTrick_ = trick;
}

void RideControlInterpreter::clearWindowState() noexcept
{
    trickWindowArmed_ = false;
    thresholdCrossed_ = false;
    windowElapsed_ = 0.0f;
    flickStart_ = clock_;
    peakX_ = 0.0f;
    peakY_ = 0.0f;
    peakMagnitude_ = 0.0f;
}

} // namespace hakui::input
