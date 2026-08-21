#include <cassert>
#include <cmath>
#include <limits>

#include "camera/ThirdPersonCameraRig.hpp"

namespace {

bool changed(float left, float right)
{
    return std::fabs(left - right) > 0.0001f;
}

} // namespace

int main()
{
    hakui::camera::ThirdPersonCameraRig camera;
    const float initialYaw = camera.yaw();
    const float initialPitch = camera.pitch();
    const float initialDistance = camera.distance();

    camera.orbit(25.0f, -12.0f);
    assert(changed(camera.yaw(), initialYaw));
    assert(changed(camera.pitch(), initialPitch));

    camera.orbit(0.0f, 100000.0f);
    assert(camera.pitch() >= 0.12f);
    camera.orbit(0.0f, -100000.0f);
    assert(camera.pitch() <= 1.18f);

    camera.zoom(1000.0f);
    assert(camera.distance() >= 3.50f);
    camera.zoom(-1000.0f);
    assert(camera.distance() <= 16.0f);

    const float shoulder = camera.shoulderSide();
    camera.toggleShoulder();
    assert(camera.shoulderSide() == -shoulder);

    camera.adjustLookSensitivity(100.0f);
    assert(camera.lookSensitivity() <= 0.016f);
    camera.adjustLookSensitivity(-100.0f);
    assert(camera.lookSensitivity() >= 0.0025f);

    camera.setFraming(-2.0f, 0.35f, 6.0f);
    assert(std::fabs(camera.yaw() + 2.0f) < 0.0001f);
    assert(std::fabs(camera.pitch() - 0.35f) < 0.0001f);
    assert(std::fabs(camera.distance() - 6.0f) < 0.0001f);

    camera.orbit(
        std::numeric_limits<float>::quiet_NaN(),
        std::numeric_limits<float>::infinity()
    );
    assert(std::isfinite(camera.yaw()));
    assert(std::isfinite(camera.pitch()));

    camera.reset();
    assert(std::fabs(camera.yaw() - initialYaw) < 0.0001f);
    assert(std::fabs(camera.pitch() - initialPitch) < 0.0001f);
    assert(std::fabs(camera.distance() - initialDistance) < 0.0001f);
    assert(camera.shoulderSide() == shoulder);
    return 0;
}
