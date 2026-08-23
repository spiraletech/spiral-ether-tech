#include <cassert>
#include <cmath>

#include "input/HakuiInput.hpp"

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
    assert(InputResolver::prompt(Action::Balance, InputDevice::KeyboardMouse) ==
           "LCTRL");
    assert(InputResolver::prompt(
        Action::CaptureExpertSnapshot,
        InputDevice::KeyboardMouse
    ) == "F12*");
    return 0;
}
