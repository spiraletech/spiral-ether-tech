#include "camera/ThirdPersonCameraRig.hpp"

#include <algorithm>
#include <cmath>

namespace hakui::camera {

namespace {

constexpr float kPi = 3.14159265358979323846f;
constexpr float kTwoPi = kPi * 2.0f;

} // namespace

ThirdPersonCameraRig::ThirdPersonCameraRig() noexcept
    : ThirdPersonCameraRig(Config{})
{
}

ThirdPersonCameraRig::ThirdPersonCameraRig(Config config) noexcept
    : config_(config)
{
    config_.minimumPitch = finiteOr(config_.minimumPitch, 0.12f);
    config_.maximumPitch = std::max(
        config_.minimumPitch,
        finiteOr(config_.maximumPitch, 1.18f)
    );
    config_.minimumDistance = std::max(
        0.1f,
        finiteOr(config_.minimumDistance, 3.50f)
    );
    config_.maximumDistance = std::max(
        config_.minimumDistance,
        finiteOr(config_.maximumDistance, 16.0f)
    );
    config_.lookSensitivity = std::clamp(
        finiteOr(config_.lookSensitivity, 0.0065f),
        0.0025f,
        0.016f
    );
    reset();
}

void ThirdPersonCameraRig::orbit(
    float horizontalDelta,
    float verticalDelta
) noexcept
{
    horizontalDelta = finiteOr(horizontalDelta, 0.0f);
    verticalDelta = finiteOr(verticalDelta, 0.0f);
    yaw_ = wrapYaw(yaw_ - horizontalDelta * config_.lookSensitivity);
    pitch_ = std::clamp(
        pitch_ - verticalDelta * config_.lookSensitivity,
        config_.minimumPitch,
        config_.maximumPitch
    );
}

void ThirdPersonCameraRig::zoom(float wheelSteps) noexcept
{
    wheelSteps = finiteOr(wheelSteps, 0.0f);
    distance_ = std::clamp(
        distance_ - wheelSteps * 0.85f,
        config_.minimumDistance,
        config_.maximumDistance
    );
}

void ThirdPersonCameraRig::reset() noexcept
{
    yaw_ = wrapYaw(finiteOr(config_.defaultYaw, 2.40f));
    pitch_ = std::clamp(
        finiteOr(config_.defaultPitch, 0.48f),
        config_.minimumPitch,
        config_.maximumPitch
    );
    distance_ = std::clamp(
        finiteOr(config_.defaultDistance, 9.50f),
        config_.minimumDistance,
        config_.maximumDistance
    );
    shoulderSide_ = 1.0f;
}

void ThirdPersonCameraRig::setFraming(
    float yaw,
    float pitch,
    float distance
) noexcept
{
    yaw_ = wrapYaw(finiteOr(yaw, yaw_));
    pitch_ = std::clamp(
        finiteOr(pitch, pitch_),
        config_.minimumPitch,
        config_.maximumPitch
    );
    distance_ = std::clamp(
        finiteOr(distance, distance_),
        config_.minimumDistance,
        config_.maximumDistance
    );
}

void ThirdPersonCameraRig::toggleShoulder() noexcept
{
    shoulderSide_ *= -1.0f;
}

void ThirdPersonCameraRig::adjustLookSensitivity(float delta) noexcept
{
    delta = finiteOr(delta, 0.0f);
    config_.lookSensitivity = std::clamp(
        config_.lookSensitivity + delta,
        0.0025f,
        0.016f
    );
}

float ThirdPersonCameraRig::yaw() const noexcept
{
    return yaw_;
}

float ThirdPersonCameraRig::pitch() const noexcept
{
    return pitch_;
}

float ThirdPersonCameraRig::distance() const noexcept
{
    return distance_;
}

float ThirdPersonCameraRig::shoulderSide() const noexcept
{
    return shoulderSide_;
}

float ThirdPersonCameraRig::lookSensitivity() const noexcept
{
    return config_.lookSensitivity;
}

float ThirdPersonCameraRig::finiteOr(float value, float fallback) noexcept
{
    return std::isfinite(value) ? value : fallback;
}

float ThirdPersonCameraRig::wrapYaw(float yaw) noexcept
{
    while (yaw > kPi) {
        yaw -= kTwoPi;
    }
    while (yaw < -kPi) {
        yaw += kTwoPi;
    }
    return yaw;
}

} // namespace hakui::camera
