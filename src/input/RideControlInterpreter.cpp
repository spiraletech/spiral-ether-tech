#include "input/RideControlInterpreter.hpp"

#include <algorithm>
#include <cmath>

namespace hakui::input {

RideControlInterpreter::RideControlInterpreter(RideControlTuning tuning)
    : tuning_(tuning)
{
    tuning_.cameraDeadzone = std::clamp(tuning_.cameraDeadzone, 0.0f, 0.95f);
    tuning_.trickDeadzone = std::clamp(tuning_.trickDeadzone, 0.0f, 0.95f);
    tuning_.flickActivationThreshold = std::clamp(
        tuning_.flickActivationThreshold,
        tuning_.trickDeadzone,
        1.0f
    );
    tuning_.flickReleaseThreshold = std::clamp(
        tuning_.flickReleaseThreshold,
        tuning_.trickDeadzone,
        tuning_.flickActivationThreshold
    );
    tuning_.minimumFlickDuration = std::max(tuning_.minimumFlickDuration, 0.0f);
    tuning_.maximumFlickDuration = std::max(
        tuning_.maximumFlickDuration,
        tuning_.minimumFlickDuration
    );
    tuning_.maximumTapDuration = std::clamp(
        tuning_.maximumTapDuration,
        tuning_.minimumFlickDuration,
        tuning_.maximumFlickDuration
    );
}

RideControlFrame RideControlInterpreter::update(
    const InputFrame& frame,
    bool rideActive,
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
    output.trick = lastResolvedTrick_;
    output.rideActive = rideActive;
    output.activationHeld = frame.action(Action::Jump).held;
    output.grind = frame.action(Action::Grind).held;
    output.balance = frame.action(Action::Balance).held;
    output.style = frame.action(Action::PrimaryAction).pressed;
    output.cancel = frame.action(Action::Cancel).pressed;
    output.spinLeft = frame.action(Action::SpinLeft).held;
    output.spinRight = frame.action(Action::SpinRight).held;
    output.propulsion = std::clamp(
        std::max(frame.axis(Axis::Accelerate), frame.action(Action::Accelerate).value),
        0.0f,
        1.0f
    );
    output.rawRightStickX = std::clamp(frame.axis(Axis::RightStickX), -1.0f, 1.0f);
    output.rawRightStickY = std::clamp(frame.axis(Axis::RightStickY), -1.0f, 1.0f);
    radialDeadzone(
        output.rawRightStickX,
        output.rawRightStickY,
        tuning_.trickDeadzone,
        output.normalizedTrickX,
        output.normalizedTrickY
    );

    if (!rideActive || frame.gamepadDisconnected) {
        reset();
        requireActivationRelease_ = output.activationHeld;
        output.rideActive = rideActive;
        output.activationHeld = false;
        output.grind = rideActive && frame.action(Action::Grind).held;
        output.balance = rideActive && frame.action(Action::Balance).held;
        output.propulsion = rideActive ? output.propulsion : 0.0f;
        diagnostics_ = output;
        return output;
    }

    if (requireActivationRelease_) {
        if (!output.activationHeld) {
            requireActivationRelease_ = false;
        }
        previousActivationHeld_ = output.activationHeld;
        diagnostics_ = output;
        return output;
    }

    const bool controllerOwnsActivation =
        frame.activeDevice == InputDevice::Gamepad && frame.gamepadAvailable;
    if (!controllerOwnsActivation) {
        if (captureState_ != CaptureState::Idle) {
            reset();
        }
        output.standardPop = frame.action(Action::Jump).pressed;
        diagnostics_ = output;
        previousActivationHeld_ = frame.action(Action::Jump).held;
        return output;
    }

    const bool activationPressed = frame.action(Action::Jump).pressed ||
        (output.activationHeld && !previousActivationHeld_);
    if (captureState_ == CaptureState::Idle && activationPressed) {
        beginCapture();
    }

    if (captureState_ == CaptureState::Capturing) {
        output.captureActive = true;
        output.rightStickOwner = RightStickOwner::TrickCapture;
        const float magnitude = std::sqrt(
            output.rawRightStickX * output.rawRightStickX +
            output.rawRightStickY * output.rawRightStickY
        );
        if (magnitude > peakMagnitude_) {
            peakMagnitude_ = magnitude;
            peakX_ = output.rawRightStickX;
            peakY_ = output.rawRightStickY;
        }
        thresholdCrossed_ = thresholdCrossed_ ||
            magnitude >= tuning_.flickActivationThreshold;

        const float duration = clock_ - captureStart_;
        const bool stickReleased = thresholdCrossed_ &&
            magnitude <= tuning_.flickReleaseThreshold;
        const bool activationReleased = !output.activationHeld;
        if (thresholdCrossed_ && (stickReleased || activationReleased)) {
            if (duration >= tuning_.minimumFlickDuration &&
                duration <= tuning_.maximumFlickDuration) {
                completeGesture(output);
            } else if (activationReleased) {
                output.standardPop = true;
            }
            captureState_ = activationReleased
                ? CaptureState::Idle
                : CaptureState::ConsumedAwaitRelease;
            output.captureActive = false;
            output.rightStickOwner = RightStickOwner::Camera;
        } else if (activationReleased) {
            output.standardPop = true;
            captureState_ = CaptureState::Idle;
            output.captureActive = false;
            output.rightStickOwner = RightStickOwner::Camera;
        }
    } else if (captureState_ == CaptureState::ConsumedAwaitRelease &&
               !output.activationHeld) {
        captureState_ = CaptureState::Idle;
    }

    previousActivationHeld_ = output.activationHeld;
    diagnostics_ = output;
    return output;
}

void RideControlInterpreter::reset() noexcept
{
    diagnostics_ = {};
    captureState_ = CaptureState::Idle;
    captureStart_ = clock_;
    peakX_ = 0.0f;
    peakY_ = 0.0f;
    peakMagnitude_ = 0.0f;
    thresholdCrossed_ = false;
    previousActivationHeld_ = false;
    requireActivationRelease_ = true;
    lastResolvedTrick_ = {};
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
    return owner == RightStickOwner::TrickCapture ? "TRICK_CAPTURE" : "CAMERA";
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

void RideControlInterpreter::beginCapture() noexcept
{
    captureState_ = CaptureState::Capturing;
    captureStart_ = clock_;
    peakX_ = 0.0f;
    peakY_ = 0.0f;
    peakMagnitude_ = 0.0f;
    thresholdCrossed_ = false;
}

void RideControlInterpreter::completeGesture(RideControlFrame& output) noexcept
{
    TrickVector trick;
    trick.direction = classifyDirection(peakX_, peakY_);
    trick.rawX = peakX_;
    trick.rawY = peakY_;
    trick.magnitude = std::clamp(peakMagnitude_, 0.0f, 1.0f);
    radialDeadzone(
        peakX_, peakY_, tuning_.trickDeadzone,
        trick.normalizedX, trick.normalizedY
    );
    trick.startTime = captureStart_;
    trick.releaseTime = clock_;
    trick.gestureDuration = clock_ - captureStart_;
    trick.valid = trick.direction != FlickDirection::None;
    output.trick = trick;
    lastResolvedTrick_ = trick;
    output.trickResolved = trick.valid;
    output.standardPop = true;
}

} // namespace hakui::input
