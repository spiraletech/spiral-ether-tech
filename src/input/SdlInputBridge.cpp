#include "input/SdlInputBridge.hpp"

#include <algorithm>
#include <cmath>

namespace hakui::input {

namespace {

constexpr std::size_t indexOf(PhysicalControl control) noexcept
{
    return static_cast<std::size_t>(control);
}

} // namespace

void SdlInputBridge::observeEvent(const SDL_Event& event) noexcept
{
    if (event.type == SDL_EVENT_KEY_DOWN && !event.key.repeat) {
        devices_.noteActivity(InputDevice::KeyboardMouse);
        latch(keyboardControl(event.key.scancode));
    } else if (event.type == SDL_EVENT_MOUSE_MOTION) {
        if (event.motion.xrel != 0.0f || event.motion.yrel != 0.0f) {
            devices_.noteActivity(InputDevice::KeyboardMouse);
        }
    } else if (event.type == SDL_EVENT_MOUSE_BUTTON_DOWN ||
               event.type == SDL_EVENT_MOUSE_WHEEL) {
        devices_.noteActivity(InputDevice::KeyboardMouse);
        if (event.type == SDL_EVENT_MOUSE_WHEEL) {
            const float direction = event.wheel.direction == SDL_MOUSEWHEEL_FLIPPED
                ? -1.0f
                : 1.0f;
            wheel_ += event.wheel.y * direction;
        }
    } else if (event.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN) {
        devices_.noteActivity(InputDevice::Gamepad);
        latch(gamepadControl(
            static_cast<SDL_GamepadButton>(event.gbutton.button)
        ));
    } else if (event.type == SDL_EVENT_GAMEPAD_AXIS_MOTION &&
               std::abs(static_cast<int>(event.gaxis.value)) > 8000) {
        devices_.noteActivity(InputDevice::Gamepad);
    }
}

void SdlInputBridge::noteGamepadOpened() noexcept
{
    devices_.setGamepadAvailable(true);
    gamepadConnected_ = true;
    gamepadDisconnected_ = false;
}

void SdlInputBridge::noteGamepadClosed() noexcept
{
    devices_.setGamepadAvailable(false);
    gamepadConnected_ = false;
    gamepadDisconnected_ = true;
    for (std::size_t index = indexOf(PhysicalControl::PadMoveX);
         index < pressed_.size(); ++index) {
        pressed_[index] = false;
    }
}

InputFrame SdlInputBridge::sample(
    SDL_Gamepad* gamepad,
    float deltaSeconds,
    bool mouseOrbiting
) noexcept
{
    (void)deltaSeconds;
    PhysicalInputFrame physical;
    physical.activeDevice = devices_.activeDevice();
    physical.controllerLayout = controllerLayout(gamepad);
    physical.gamepadAvailable = gamepad != nullptr && devices_.gamepadAvailable();
    physical.gamepadConnected = gamepadConnected_;
    physical.gamepadDisconnected = gamepadDisconnected_;

    const bool* keys = SDL_GetKeyboardState(nullptr);
    const auto setKey = [&](PhysicalControl control, SDL_Scancode scancode) {
        physical.set(control, keys[scancode] ? 1.0f : 0.0f, pressed_[indexOf(control)]);
    };
    setKey(PhysicalControl::KeyMoveLeft, SDL_SCANCODE_A);
    setKey(PhysicalControl::KeyMoveRight, SDL_SCANCODE_D);
    setKey(PhysicalControl::KeyMoveForward, SDL_SCANCODE_W);
    setKey(PhysicalControl::KeyMoveBackward, SDL_SCANCODE_S);
    setKey(PhysicalControl::KeyJump, SDL_SCANCODE_SPACE);
    physical.set(
        PhysicalControl::KeyAccelerate,
        (keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT]) ? 1.0f : 0.0f,
        pressed_[indexOf(PhysicalControl::KeyAccelerate)]
    );
    setKey(PhysicalControl::KeyInteract, SDL_SCANCODE_E);
    setKey(PhysicalControl::KeyGuard, SDL_SCANCODE_Q);
    setKey(PhysicalControl::KeyContext, SDL_SCANCODE_F);
    physical.set(
        PhysicalControl::KeyBalance,
        (keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL]) ? 1.0f : 0.0f,
        pressed_[indexOf(PhysicalControl::KeyBalance)]
    );
    setKey(PhysicalControl::KeyCancel, SDL_SCANCODE_C);
    physical.set(
        PhysicalControl::KeyRecover,
        (keys[SDL_SCANCODE_R] || keys[SDL_SCANCODE_K]) ? 1.0f : 0.0f,
        pressed_[indexOf(PhysicalControl::KeyRecover)]
    );
    setKey(PhysicalControl::KeyPrimaryLegacy, SDL_SCANCODE_Z);
    setKey(PhysicalControl::KeySecondaryLegacy, SDL_SCANCODE_X);
    setKey(PhysicalControl::KeyGuardLegacy, SDL_SCANCODE_V);
    setKey(PhysicalControl::KeyBalanceLegacy, SDL_SCANCODE_M);
    setKey(PhysicalControl::KeyPause, SDL_SCANCODE_ESCAPE);
    setKey(PhysicalControl::KeyQuit, SDL_SCANCODE_F10);
    setKey(PhysicalControl::KeyExpertSnapshot, SDL_SCANCODE_F12);
    setKey(PhysicalControl::KeyModeOnFoot, SDL_SCANCODE_1);
    setKey(PhysicalControl::KeyModeSkateboard, SDL_SCANCODE_2);
    setKey(PhysicalControl::KeyModeBmx, SDL_SCANCODE_3);
    setKey(PhysicalControl::KeyModeCar, SDL_SCANCODE_4);
    setKey(PhysicalControl::KeyLookSlower, SDL_SCANCODE_LEFTBRACKET);
    setKey(PhysicalControl::KeyLookFaster, SDL_SCANCODE_RIGHTBRACKET);
    setKey(PhysicalControl::KeyVolumeDown, SDL_SCANCODE_MINUS);
    setKey(PhysicalControl::KeyVolumeUp, SDL_SCANCODE_EQUALS);
    setKey(PhysicalControl::KeyToggleRideFallback, SDL_SCANCODE_F9);

    float mouseX = 0.0f;
    float mouseY = 0.0f;
    const SDL_MouseButtonFlags mouseButtons = SDL_GetRelativeMouseState(
        &mouseX,
        &mouseY
    );
    physical.set(
        PhysicalControl::MouseOrbit,
        (mouseButtons & SDL_BUTTON_RMASK) != 0 ? 1.0f : 0.0f
    );
    if (mouseOrbiting) {
        physical.set(PhysicalControl::MouseLookX, mouseX);
        physical.set(PhysicalControl::MouseLookY, mouseY);
    }
    physical.set(PhysicalControl::MouseWheel, wheel_);

    if (gamepad) {
        physical.set(
            PhysicalControl::PadMoveX,
            normalizedStick(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTX))
        );
        physical.set(
            PhysicalControl::PadMoveY,
            -normalizedStick(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFTY))
        );
        physical.set(
            PhysicalControl::PadLookX,
            rawStick(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTX))
        );
        physical.set(
            PhysicalControl::PadLookY,
            rawStick(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHTY))
        );
        physical.set(
            PhysicalControl::PadLeftTrigger,
            normalizedTrigger(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_LEFT_TRIGGER))
        );
        physical.set(
            PhysicalControl::PadRightTrigger,
            normalizedTrigger(SDL_GetGamepadAxis(gamepad, SDL_GAMEPAD_AXIS_RIGHT_TRIGGER))
        );

        const auto setPad = [&](PhysicalControl control, SDL_GamepadButton button) {
            physical.set(
                control,
                SDL_GetGamepadButton(gamepad, button) ? 1.0f : 0.0f,
                pressed_[indexOf(control)]
            );
        };
        setPad(PhysicalControl::PadSouth, SDL_GAMEPAD_BUTTON_SOUTH);
        setPad(PhysicalControl::PadEast, SDL_GAMEPAD_BUTTON_EAST);
        setPad(PhysicalControl::PadWest, SDL_GAMEPAD_BUTTON_WEST);
        setPad(PhysicalControl::PadNorth, SDL_GAMEPAD_BUTTON_NORTH);
        setPad(PhysicalControl::PadLeftShoulder, SDL_GAMEPAD_BUTTON_LEFT_SHOULDER);
        setPad(PhysicalControl::PadRightShoulder, SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER);
        setPad(PhysicalControl::PadLeftStick, SDL_GAMEPAD_BUTTON_LEFT_STICK);
        setPad(PhysicalControl::PadRightStick, SDL_GAMEPAD_BUTTON_RIGHT_STICK);
        setPad(PhysicalControl::PadStart, SDL_GAMEPAD_BUTTON_START);
        setPad(PhysicalControl::PadDpadUp, SDL_GAMEPAD_BUTTON_DPAD_UP);
        setPad(PhysicalControl::PadDpadDown, SDL_GAMEPAD_BUTTON_DPAD_DOWN);
        setPad(PhysicalControl::PadDpadLeft, SDL_GAMEPAD_BUTTON_DPAD_LEFT);
        setPad(PhysicalControl::PadDpadRight, SDL_GAMEPAD_BUTTON_DPAD_RIGHT);
    }

    InputFrame frame = resolver_.resolve(physical);
    pressed_.fill(false);
    wheel_ = 0.0f;
    gamepadConnected_ = false;
    gamepadDisconnected_ = false;
    return frame;
}

InputDevice SdlInputBridge::activeDevice() const noexcept
{
    return devices_.activeDevice();
}

bool SdlInputBridge::gamepadAvailable() const noexcept
{
    return devices_.gamepadAvailable();
}

PhysicalControl SdlInputBridge::keyboardControl(SDL_Scancode scancode) noexcept
{
    switch (scancode) {
        case SDL_SCANCODE_A: return PhysicalControl::KeyMoveLeft;
        case SDL_SCANCODE_D: return PhysicalControl::KeyMoveRight;
        case SDL_SCANCODE_W: return PhysicalControl::KeyMoveForward;
        case SDL_SCANCODE_S: return PhysicalControl::KeyMoveBackward;
        case SDL_SCANCODE_SPACE: return PhysicalControl::KeyJump;
        case SDL_SCANCODE_LSHIFT:
        case SDL_SCANCODE_RSHIFT: return PhysicalControl::KeyAccelerate;
        case SDL_SCANCODE_E: return PhysicalControl::KeyInteract;
        case SDL_SCANCODE_Q: return PhysicalControl::KeyGuard;
        case SDL_SCANCODE_F: return PhysicalControl::KeyContext;
        case SDL_SCANCODE_LCTRL:
        case SDL_SCANCODE_RCTRL: return PhysicalControl::KeyBalance;
        case SDL_SCANCODE_C: return PhysicalControl::KeyCancel;
        case SDL_SCANCODE_R:
        case SDL_SCANCODE_K: return PhysicalControl::KeyRecover;
        case SDL_SCANCODE_Z: return PhysicalControl::KeyPrimaryLegacy;
        case SDL_SCANCODE_X: return PhysicalControl::KeySecondaryLegacy;
        case SDL_SCANCODE_V: return PhysicalControl::KeyGuardLegacy;
        case SDL_SCANCODE_M: return PhysicalControl::KeyBalanceLegacy;
        case SDL_SCANCODE_ESCAPE: return PhysicalControl::KeyPause;
        case SDL_SCANCODE_F10: return PhysicalControl::KeyQuit;
        case SDL_SCANCODE_F12: return PhysicalControl::KeyExpertSnapshot;
        case SDL_SCANCODE_1: return PhysicalControl::KeyModeOnFoot;
        case SDL_SCANCODE_2: return PhysicalControl::KeyModeSkateboard;
        case SDL_SCANCODE_3: return PhysicalControl::KeyModeBmx;
        case SDL_SCANCODE_4: return PhysicalControl::KeyModeCar;
        case SDL_SCANCODE_LEFTBRACKET: return PhysicalControl::KeyLookSlower;
        case SDL_SCANCODE_RIGHTBRACKET: return PhysicalControl::KeyLookFaster;
        case SDL_SCANCODE_MINUS: return PhysicalControl::KeyVolumeDown;
        case SDL_SCANCODE_EQUALS: return PhysicalControl::KeyVolumeUp;
        case SDL_SCANCODE_F9: return PhysicalControl::KeyToggleRideFallback;
        default: return PhysicalControl::Count;
    }
}

PhysicalControl SdlInputBridge::gamepadControl(
    SDL_GamepadButton button
) noexcept
{
    switch (button) {
        case SDL_GAMEPAD_BUTTON_SOUTH: return PhysicalControl::PadSouth;
        case SDL_GAMEPAD_BUTTON_EAST: return PhysicalControl::PadEast;
        case SDL_GAMEPAD_BUTTON_WEST: return PhysicalControl::PadWest;
        case SDL_GAMEPAD_BUTTON_NORTH: return PhysicalControl::PadNorth;
        case SDL_GAMEPAD_BUTTON_LEFT_SHOULDER:
            return PhysicalControl::PadLeftShoulder;
        case SDL_GAMEPAD_BUTTON_RIGHT_SHOULDER:
            return PhysicalControl::PadRightShoulder;
        case SDL_GAMEPAD_BUTTON_LEFT_STICK: return PhysicalControl::PadLeftStick;
        case SDL_GAMEPAD_BUTTON_RIGHT_STICK: return PhysicalControl::PadRightStick;
        case SDL_GAMEPAD_BUTTON_START: return PhysicalControl::PadStart;
        case SDL_GAMEPAD_BUTTON_DPAD_UP: return PhysicalControl::PadDpadUp;
        case SDL_GAMEPAD_BUTTON_DPAD_DOWN: return PhysicalControl::PadDpadDown;
        case SDL_GAMEPAD_BUTTON_DPAD_LEFT: return PhysicalControl::PadDpadLeft;
        case SDL_GAMEPAD_BUTTON_DPAD_RIGHT: return PhysicalControl::PadDpadRight;
        default: return PhysicalControl::Count;
    }
}

float SdlInputBridge::normalizedStick(Sint16 raw) noexcept
{
    constexpr float deadZone = 0.16f;
    const float value = std::clamp(
        static_cast<float>(raw) / 32767.0f,
        -1.0f,
        1.0f
    );
    const float magnitude = std::fabs(value);
    if (magnitude <= deadZone) {
        return 0.0f;
    }
    return std::copysign(
        (magnitude - deadZone) / (1.0f - deadZone),
        value
    );
}

float SdlInputBridge::rawStick(Sint16 raw) noexcept
{
    return std::clamp(
        static_cast<float>(raw) / 32767.0f,
        -1.0f,
        1.0f
    );
}

float SdlInputBridge::normalizedTrigger(Sint16 raw) noexcept
{
    return std::clamp(static_cast<float>(raw) / 32767.0f, 0.0f, 1.0f);
}

ControllerLayout SdlInputBridge::controllerLayout(SDL_Gamepad* gamepad) noexcept
{
    if (!gamepad) {
        return ControllerLayout::Generic;
    }
    switch (SDL_GetGamepadType(gamepad)) {
        case SDL_GAMEPAD_TYPE_XBOX360:
        case SDL_GAMEPAD_TYPE_XBOXONE:
            return ControllerLayout::Xbox;
        case SDL_GAMEPAD_TYPE_PS3:
        case SDL_GAMEPAD_TYPE_PS4:
        case SDL_GAMEPAD_TYPE_PS5:
            return ControllerLayout::PlayStation;
        default:
            return ControllerLayout::Generic;
    }
}

void SdlInputBridge::latch(PhysicalControl control) noexcept
{
    if (control != PhysicalControl::Count) {
        pressed_[indexOf(control)] = true;
    }
}

} // namespace hakui::input
