#include <cassert>
#include <cmath>

#include "input/HakuiInput.hpp"
#include "input/RideControlInterpreter.hpp"

namespace {

bool nearlyEqual(float left, float right)
{
    return std::fabs(left - right) < 0.001f;
}

} // namespace

int main()
{
    using namespace hakui::input;

    InputResolver resolver;
    PhysicalInputFrame keyboard;
    keyboard.set(PhysicalControl::KeyMoveForward, 1.0f);
    keyboard.set(PhysicalControl::KeyMoveRight, 1.0f);
    keyboard.set(PhysicalControl::KeyJump, 1.0f, true);
    keyboard.set(PhysicalControl::KeyGuard, 1.0f);
    keyboard.set(PhysicalControl::KeyContext, 1.0f);
    keyboard.set(PhysicalControl::KeyBalance, 1.0f);
    const InputFrame keyboardFrame = resolver.resolve(keyboard);
    assert(nearlyEqual(keyboardFrame.axis(Axis::MoveForward), 1.0f));
    assert(nearlyEqual(keyboardFrame.axis(Axis::MoveRight), 1.0f));
    assert(keyboardFrame.action(Action::Jump).held);
    assert(keyboardFrame.action(Action::Jump).pressed);
    assert(keyboardFrame.action(Action::Guard).held);
    assert(keyboardFrame.action(Action::Grind).held);
    assert(keyboardFrame.action(Action::Balance).held);

    PhysicalInputFrame gamepad;
    gamepad.activeDevice = InputDevice::Gamepad;
    gamepad.gamepadAvailable = true;
    gamepad.set(PhysicalControl::PadMoveX, -0.45f);
    gamepad.set(PhysicalControl::PadMoveY, 0.75f);
    gamepad.set(PhysicalControl::PadSouth, 1.0f, true);
    gamepad.set(PhysicalControl::PadNorth, 1.0f, true);
    gamepad.set(PhysicalControl::PadRightShoulder, 1.0f);
    gamepad.set(PhysicalControl::PadLeftShoulder, 1.0f);
    gamepad.set(PhysicalControl::PadLeftTrigger, 0.65f);
    gamepad.set(PhysicalControl::PadRightTrigger, 0.82f);
    const InputFrame padFrame = resolver.resolve(gamepad);
    assert(padFrame.activeDevice == InputDevice::Gamepad);
    assert(nearlyEqual(padFrame.axis(Axis::MoveRight), -0.45f));
    assert(nearlyEqual(padFrame.axis(Axis::MoveForward), 0.75f));
    assert(padFrame.action(Action::Jump).pressed);
    assert(padFrame.action(Action::Grind).held);
    assert(padFrame.action(Action::Balance).held);
    assert(padFrame.action(Action::SpinLeft).held);
    assert(padFrame.action(Action::SpinRight).held);
    assert(padFrame.action(Action::Accelerate).held);
    assert(nearlyEqual(padFrame.axis(Axis::Accelerate), 0.82f));

    const DisciplineIntent foot = DisciplineInterpreter::interpret(
        padFrame,
        ActiveDiscipline::OnFoot
    );
    const DisciplineIntent skateboard = DisciplineInterpreter::interpret(
        padFrame,
        ActiveDiscipline::Skateboard
    );
    const DisciplineIntent bmx = DisciplineInterpreter::interpret(
        padFrame,
        ActiveDiscipline::Bmx
    );
    const DisciplineIntent seated = DisciplineInterpreter::interpret(
        padFrame,
        ActiveDiscipline::Seated
    );
    assert(foot.traversal == TraversalIntent::Jump);
    assert(skateboard.traversal == TraversalIntent::Ollie);
    assert(bmx.traversal == TraversalIntent::BunnyHop);
    assert(seated.traversal == TraversalIntent::None);
    assert(skateboard.grind);
    assert(skateboard.balance);
    assert(!foot.grind);
    assert(!foot.balance);

    DeviceActivityTracker devices;
    assert(devices.activeDevice() == InputDevice::KeyboardMouse);
    devices.noteActivity(InputDevice::Gamepad);
    assert(devices.activeDevice() == InputDevice::KeyboardMouse);
    devices.setGamepadAvailable(true);
    devices.noteActivity(InputDevice::Gamepad);
    assert(devices.activeDevice() == InputDevice::Gamepad);
    devices.noteActivity(InputDevice::KeyboardMouse);
    assert(devices.activeDevice() == InputDevice::KeyboardMouse);
    devices.noteActivity(InputDevice::Gamepad);
    devices.setGamepadAvailable(false);
    assert(devices.activeDevice() == InputDevice::KeyboardMouse);
    assert(!devices.gamepadAvailable());

    assert(InputResolver::prompt(Action::Interact, InputDevice::KeyboardMouse) == "E");
    assert(InputResolver::prompt(Action::Interact, InputDevice::Gamepad) == "WEST");
    assert(InputResolver::prompt(Action::Jump, InputDevice::Gamepad) == "SOUTH");
    assert(InputResolver::prompt(
        Action::Jump,
        InputDevice::Gamepad,
        ControllerLayout::PlayStation
    ) == "CROSS");
    assert(InputResolver::prompt(
        Action::Grind,
        InputDevice::Gamepad,
        ControllerLayout::PlayStation
    ) == "TRIANGLE");
    assert(InputResolver::prompt(
        Action::Jump,
        InputDevice::Gamepad,
        ControllerLayout::Xbox
    ) == "A");
    assert(InputResolver::prompt(
        Action::Grind,
        InputDevice::Gamepad,
        ControllerLayout::Xbox
    ) == "Y");
    assert(InputResolver::prompt(
        Action::Grind,
        InputDevice::Gamepad,
        ControllerLayout::Generic
    ) == "NORTH");
    assert(InputResolver::prompt(
        Action::SpinLeft,
        InputDevice::Gamepad,
        ControllerLayout::PlayStation
    ) == "L1");
    assert(InputResolver::prompt(
        Action::Accelerate,
        InputDevice::Gamepad,
        ControllerLayout::Xbox
    ) == "RT");
    assert(InputResolver::prompt(Action::Balance, InputDevice::KeyboardMouse) ==
           "LCTRL");
    assert(InputResolver::prompt(
        Action::CaptureExpertSnapshot,
        InputDevice::KeyboardMouse
    ) == "F12*");

    constexpr struct DirectionCase {
        float x;
        float y;
        FlickDirection expected;
    } directions[] = {
        {-1.0f, 0.0f, FlickDirection::Left},
        {1.0f, 0.0f, FlickDirection::Right},
        {0.0f, -1.0f, FlickDirection::Up},
        {0.0f, 1.0f, FlickDirection::Down},
        {-0.8f, -0.8f, FlickDirection::UpLeft},
        {0.8f, -0.8f, FlickDirection::UpRight},
        {-0.8f, 0.8f, FlickDirection::DownLeft},
        {0.8f, 0.8f, FlickDirection::DownRight}
    };
    for (const DirectionCase& direction : directions) {
        assert(RideControlInterpreter::classifyDirection(
            direction.x,
            direction.y
        ) == direction.expected);
    }
    float deadzoneX = 1.0f;
    float deadzoneY = 1.0f;
    RideControlInterpreter::radialDeadzone(
        0.10f, 0.10f, 0.22f, deadzoneX, deadzoneY
    );
    assert(nearlyEqual(deadzoneX, 0.0f));
    assert(nearlyEqual(deadzoneY, 0.0f));
    RideControlInterpreter::radialDeadzone(
        0.80f, 0.0f, 0.20f, deadzoneX, deadzoneY
    );
    assert(nearlyEqual(deadzoneX, 0.75f));
    assert(nearlyEqual(deadzoneY, 0.0f));

    RideControlInterpreter rideControls;
    PhysicalInputFrame popPress;
    popPress.activeDevice = InputDevice::Gamepad;
    popPress.gamepadAvailable = true;
    // A backend may collapse a quick tap into one sample: pressed, no longer
    // held. This remains an immediate normal pop.
    popPress.set(PhysicalControl::PadSouth, 0.0f, true);
    RideControlFrame rideFrame = rideControls.update(
        resolver.resolve(popPress),
        true,
        false,
        0.016f
    );
    assert(rideFrame.popIntent);
    assert(!rideFrame.trickWindowArmed);
    assert(rideFrame.rightStickOwner == RightStickOwner::Camera);

    // A physical hold preloads without opening the trick window. Release
    // emits one capped pop; the RS still belongs to the camera until gameplay
    // confirms airborne state.
    RideControlInterpreter preloadControls;
    PhysicalInputFrame preloadPress;
    preloadPress.activeDevice = InputDevice::Gamepad;
    preloadPress.gamepadAvailable = true;
    preloadPress.set(PhysicalControl::PadSouth, 1.0f, true);
    RideControlFrame preloadFrame = preloadControls.update(
        resolver.resolve(preloadPress), true, false, 0.10f
    );
    assert(!preloadFrame.popIntent);
    assert(preloadFrame.popPreparing);
    assert(preloadFrame.popPreload > 0.0f);
    PhysicalInputFrame preloadHeld = preloadPress;
    preloadHeld.pressed.fill(false);
    for (int frameIndex = 0; frameIndex < 8; ++frameIndex) {
        preloadFrame = preloadControls.update(
            resolver.resolve(preloadHeld), true, false, 0.10f
        );
    }
    assert(preloadFrame.popPreload == 1.0f);
    PhysicalInputFrame preloadRelease;
    preloadRelease.activeDevice = InputDevice::Gamepad;
    preloadRelease.gamepadAvailable = true;
    preloadFrame = preloadControls.update(
        resolver.resolve(preloadRelease), true, false, 0.016f
    );
    assert(preloadFrame.popIntent);
    assert(!preloadFrame.popPreparing);
    assert(preloadFrame.popPreload == 1.0f);
    assert(!preloadFrame.trickWindowArmed);

    // Gameplay confirms that the discrete pop actually left the ground.
    rideControls.armTrickWindow();
    PhysicalInputFrame released;
    released.activeDevice = InputDevice::Gamepad;
    released.gamepadAvailable = true;
    rideFrame = rideControls.update(
        resolver.resolve(released),
        true,
        true,
        0.03f
    );
    assert(!rideFrame.popIntent);
    assert(rideFrame.trickWindowArmed);
    assert(rideFrame.trickWindowListening);
    assert(rideFrame.rightStickOwner == RightStickOwner::TrickWindow);

    PhysicalInputFrame flickPeak;
    flickPeak.activeDevice = InputDevice::Gamepad;
    flickPeak.gamepadAvailable = true;
    flickPeak.set(PhysicalControl::PadLookX, 0.92f);
    rideFrame = rideControls.update(
        resolver.resolve(flickPeak),
        true,
        true,
        0.06f
    );
    assert(rideFrame.trickWindowArmed);
    assert(!rideFrame.trickIntent);

    rideFrame = rideControls.update(
        resolver.resolve(released),
        true,
        true,
        0.06f
    );
    assert(rideFrame.trickIntent);
    assert(rideFrame.trick.direction == FlickDirection::Right);
    assert(!rideFrame.trickWindowArmed);
    assert(rideFrame.rightStickOwner == RightStickOwner::Camera);

    // A flick beginning on the first frame after pop is buffered through the
    // tiny post-launch delay; South is already released throughout.
    rideFrame = rideControls.update(
        resolver.resolve(popPress),
        true,
        false,
        0.016f
    );
    assert(rideFrame.popIntent);
    rideControls.armTrickWindow();
    rideFrame = rideControls.update(
        resolver.resolve(flickPeak),
        true,
        true,
        0.016f
    );
    assert(!rideFrame.popIntent);
    assert(rideFrame.rightStickOwner == RightStickOwner::TrickWindow);
    rideFrame = rideControls.update(
        resolver.resolve(released),
        true,
        true,
        0.016f
    );
    assert(rideFrame.trickIntent);
    assert(rideFrame.trick.direction == FlickDirection::Right);

    // A pop without a flick remains a normal ollie/bunny hop and times out.
    rideFrame = rideControls.update(
        resolver.resolve(popPress),
        true,
        false,
        0.016f
    );
    assert(rideFrame.popIntent);
    rideControls.armTrickWindow();
    for (int frameIndex = 0; frameIndex < 9; ++frameIndex) {
        rideFrame = rideControls.update(
            resolver.resolve(released),
            true,
            true,
            0.10f
        );
    }
    assert(!rideFrame.trickIntent);
    assert(!rideFrame.trickWindowArmed);
    assert(rideFrame.rightStickOwner == RightStickOwner::Camera);

    // Grounded or late right-stick motion is camera input, never a trick.
    rideFrame = rideControls.update(
        resolver.resolve(flickPeak),
        true,
        false,
        0.016f
    );
    assert(!rideFrame.trickIntent);
    assert(rideFrame.rightStickOwner == RightStickOwner::Camera);

    // The first decisive flick near the end of the window is still accepted.
    rideControls.armTrickWindow();
    for (int frameIndex = 0; frameIndex < 6; ++frameIndex) {
        rideFrame = rideControls.update(
            resolver.resolve(released),
            true,
            true,
            0.10f
        );
    }
    rideFrame = rideControls.update(
        resolver.resolve(flickPeak),
        true,
        true,
        0.05f
    );
    assert(!rideFrame.trickIntent);
    rideFrame = rideControls.update(
        resolver.resolve(released),
        true,
        true,
        0.04f
    );
    assert(rideFrame.trickIntent);
    assert(rideFrame.rightStickOwner == RightStickOwner::Camera);

    // Landing, explicit close, disconnect, dismount, and reset all restore camera.
    rideControls.armTrickWindow();
    rideFrame = rideControls.update(
        resolver.resolve(released),
        true,
        false,
        0.016f
    );
    assert(!rideFrame.trickWindowArmed);
    assert(rideFrame.rightStickOwner == RightStickOwner::Camera);

    rideControls.armTrickWindow();
    rideControls.closeTrickWindow();
    assert(!rideControls.diagnostics().trickWindowArmed);
    assert(rideControls.diagnostics().rightStickOwner == RightStickOwner::Camera);

    PhysicalInputFrame disconnected;
    disconnected.activeDevice = InputDevice::Gamepad;
    disconnected.gamepadDisconnected = true;
    rideControls.armTrickWindow();
    rideFrame = rideControls.update(
        resolver.resolve(disconnected),
        true,
        true,
        0.016f
    );
    assert(!rideFrame.trickWindowArmed);
    assert(rideFrame.rightStickOwner == RightStickOwner::Camera);

    rideControls.armTrickWindow();
    rideFrame = rideControls.update(
        resolver.resolve(released),
        false,
        false,
        0.016f
    );
    assert(!rideFrame.trickWindowArmed);
    rideControls.reset();
    assert(rideControls.diagnostics().rightStickOwner == RightStickOwner::Camera);
    return 0;
}
