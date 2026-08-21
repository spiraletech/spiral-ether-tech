#pragma once

namespace hakui::camera {

class ThirdPersonCameraRig {
public:
    struct Config {
        float defaultYaw = 2.40f;
        float defaultPitch = 0.48f;
        float defaultDistance = 9.50f;
        float minimumPitch = 0.12f;
        float maximumPitch = 1.18f;
        float minimumDistance = 3.50f;
        float maximumDistance = 16.0f;
        float lookSensitivity = 0.0065f;
    };

    ThirdPersonCameraRig() noexcept;
    explicit ThirdPersonCameraRig(Config config) noexcept;

    void orbit(float horizontalDelta, float verticalDelta) noexcept;
    void zoom(float wheelSteps) noexcept;
    void reset() noexcept;
    void setFraming(float yaw, float pitch, float distance) noexcept;
    void toggleShoulder() noexcept;
    void adjustLookSensitivity(float delta) noexcept;

    float yaw() const noexcept;
    float pitch() const noexcept;
    float distance() const noexcept;
    float shoulderSide() const noexcept;
    float lookSensitivity() const noexcept;

private:
    static float finiteOr(float value, float fallback) noexcept;
    static float wrapYaw(float yaw) noexcept;

    Config config_{};
    float yaw_ = 2.40f;
    float pitch_ = 0.48f;
    float distance_ = 9.50f;
    float shoulderSide_ = 1.0f;
};

} // namespace hakui::camera
