#pragma once

#include <array>

#include <SDL3/SDL.h>

#include "input/HakuiInput.hpp"

namespace hakui::input {

// Native boundary: SDL hardware state enters here and leaves as a semantic
// InputFrame. Gameplay systems never receive scancodes or gamepad button names.
class SdlInputBridge {
public:
    void observeEvent(const SDL_Event& event) noexcept;
    void noteGamepadOpened() noexcept;
    void noteGamepadClosed() noexcept;

    InputFrame sample(
        SDL_Gamepad* gamepad,
        float deltaSeconds,
        bool mouseOrbiting
    ) noexcept;

    InputDevice activeDevice() const noexcept;
    bool gamepadAvailable() const noexcept;

private:
    static PhysicalControl keyboardControl(SDL_Scancode scancode) noexcept;
    static PhysicalControl gamepadControl(SDL_GamepadButton button) noexcept;
    static float normalizedStick(Sint16 raw) noexcept;
    static float rawStick(Sint16 raw) noexcept;
    static float normalizedTrigger(Sint16 raw) noexcept;
    static ControllerLayout controllerLayout(SDL_Gamepad* gamepad) noexcept;

    void latch(PhysicalControl control) noexcept;

    DeviceActivityTracker devices_{};
    InputResolver resolver_{};
    std::array<bool, physicalControlCount> pressed_{};
    float wheel_ = 0.0f;
    bool gamepadConnected_ = false;
    bool gamepadDisconnected_ = false;
};

} // namespace hakui::input
