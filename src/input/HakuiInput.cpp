#include "input/HakuiInput.hpp"

#include <algorithm>
#include <cmath>

namespace hakui::input {

namespace {

struct ActionBinding {
    Action action;
    PhysicalControl control;
};

struct AxisBinding {
    Axis axis;
    PhysicalControl control;
    float scale;
};

constexpr auto kActionBindings = std::to_array<ActionBinding>({
    {Action::Jump, PhysicalControl::KeyJump},
    {Action::Jump, PhysicalControl::PadSouth},
    {Action::Interact, PhysicalControl::KeyInteract},
    {Action::Interact, PhysicalControl::PadWest},
    {Action::Cancel, PhysicalControl::KeyCancel},
    {Action::Cancel, PhysicalControl::PadEast},
    {Action::Dismount, PhysicalControl::KeyCancel},
    {Action::Dismount, PhysicalControl::PadEast},
    {Action::PrimaryAction, PhysicalControl::KeyPrimaryLegacy},
    {Action::PrimaryAction, PhysicalControl::PadWest},
    {Action::SecondaryAction, PhysicalControl::KeySecondaryLegacy},
    {Action::SecondaryAction, PhysicalControl::PadNorth},
    {Action::Guard, PhysicalControl::KeyGuard},
    {Action::Guard, PhysicalControl::KeyGuardLegacy},
    {Action::Guard, PhysicalControl::PadLeftTrigger},
    {Action::Context, PhysicalControl::KeyContext},
    {Action::Grind, PhysicalControl::KeyContext},
    {Action::Grind, PhysicalControl::PadNorth},
    {Action::Balance, PhysicalControl::KeyBalance},
    {Action::Balance, PhysicalControl::KeyBalanceLegacy},
    {Action::Balance, PhysicalControl::PadLeftTrigger},
    {Action::Accelerate, PhysicalControl::KeyAccelerate},
    {Action::Accelerate, PhysicalControl::PadRightTrigger},
    {Action::SpinLeft, PhysicalControl::PadLeftShoulder},
    {Action::SpinRight, PhysicalControl::PadRightShoulder},
    {Action::Recover, PhysicalControl::KeyRecover},
    {Action::Recover, PhysicalControl::PadRightStick},
    {Action::Pause, PhysicalControl::KeyPause},
    {Action::Pause, PhysicalControl::PadStart},
    {Action::Quit, PhysicalControl::KeyQuit},
    {Action::OrbitCamera, PhysicalControl::MouseOrbit},
    {Action::CameraReset, PhysicalControl::KeyRecover},
    {Action::CameraReset, PhysicalControl::PadRightStick},
    {Action::SelectOnFoot, PhysicalControl::KeyModeOnFoot},
    {Action::SelectOnFoot, PhysicalControl::PadDpadUp},
    {Action::SelectSkateboard, PhysicalControl::KeyModeSkateboard},
    {Action::SelectSkateboard, PhysicalControl::PadDpadLeft},
    {Action::SelectBmx, PhysicalControl::KeyModeBmx},
    {Action::SelectBmx, PhysicalControl::PadDpadRight},
    {Action::SelectCar, PhysicalControl::KeyModeCar},
    {Action::LookSlower, PhysicalControl::KeyLookSlower},
    {Action::LookFaster, PhysicalControl::KeyLookFaster},
    {Action::VolumeDown, PhysicalControl::KeyVolumeDown},
    {Action::VolumeUp, PhysicalControl::KeyVolumeUp},
    {Action::ToggleRideFallback, PhysicalControl::KeyToggleRideFallback},
    {Action::CaptureExpertSnapshot, PhysicalControl::KeyExpertSnapshot}
});

constexpr auto kAxisBindings = std::to_array<AxisBinding>({
    {Axis::MoveRight, PhysicalControl::KeyMoveLeft, -1.0f},
    {Axis::MoveRight, PhysicalControl::KeyMoveRight, 1.0f},
    {Axis::MoveRight, PhysicalControl::PadMoveX, 1.0f},
    {Axis::MoveForward, PhysicalControl::KeyMoveForward, 1.0f},
    {Axis::MoveForward, PhysicalControl::KeyMoveBackward, -1.0f},
    {Axis::MoveForward, PhysicalControl::PadMoveY, 1.0f},
    {Axis::LookRight, PhysicalControl::MouseLookX, 1.0f},
    {Axis::LookDown, PhysicalControl::MouseLookY, 1.0f},
    {Axis::RightStickX, PhysicalControl::PadLookX, 1.0f},
    {Axis::RightStickY, PhysicalControl::PadLookY, 1.0f},
    {Axis::Zoom, PhysicalControl::MouseWheel, 1.0f},
    {Axis::Accelerate, PhysicalControl::KeyAccelerate, 1.0f},
    {Axis::Accelerate, PhysicalControl::PadRightTrigger, 1.0f}
});

constexpr std::size_t indexOf(PhysicalControl control) noexcept
{
    return static_cast<std::size_t>(control);
}

constexpr std::size_t indexOf(Action action) noexcept
{
    return static_cast<std::size_t>(action);
}

constexpr std::size_t indexOf(Axis axis) noexcept
{
    return static_cast<std::size_t>(axis);
}

} // namespace

void PhysicalInputFrame::set(
    PhysicalControl control,
    float newValue,
    bool wasPressedValue
) noexcept
{
    const std::size_t index = indexOf(control);
    values[index] = std::isfinite(newValue) ? newValue : 0.0f;
    pressed[index] = pressed[index] || wasPressedValue;
}

float PhysicalInputFrame::value(PhysicalControl control) const noexcept
{
    return values[indexOf(control)];
}

bool PhysicalInputFrame::wasPressed(PhysicalControl control) const noexcept
{
    return pressed[indexOf(control)];
}

const ActionState& InputFrame::action(Action semantic) const noexcept
{
    return actions[indexOf(semantic)];
}

float InputFrame::axis(Axis semantic) const noexcept
{
    return axes[indexOf(semantic)];
}

DisciplineIntent DisciplineInterpreter::interpret(
    const InputFrame& frame,
    ActiveDiscipline discipline
) noexcept
{
    DisciplineIntent intent;
    intent.moveRight = frame.axis(Axis::MoveRight);
    intent.moveForward = frame.axis(Axis::MoveForward);
    intent.accelerate = std::max(
        frame.axis(Axis::Accelerate),
        frame.action(Action::Accelerate).value
    );
    intent.interact = frame.action(Action::Interact).pressed;
    intent.cancel = frame.action(Action::Cancel).pressed;
    intent.primary = frame.action(Action::PrimaryAction).pressed;
    intent.secondary = frame.action(Action::SecondaryAction).pressed;
    intent.guard = frame.action(Action::Guard).held;
    intent.recover = frame.action(Action::Recover).pressed;

    if (frame.action(Action::Jump).pressed) {
        switch (discipline) {
            case ActiveDiscipline::OnFoot:
                intent.traversal = TraversalIntent::Jump;
                break;
            case ActiveDiscipline::Skateboard:
                intent.traversal = TraversalIntent::Ollie;
                break;
            case ActiveDiscipline::Bmx:
                intent.traversal = TraversalIntent::BunnyHop;
                break;
            case ActiveDiscipline::Boxing:
            case ActiveDiscipline::Seated:
                break;
        }
    }

    const bool rideDiscipline = discipline == ActiveDiscipline::Skateboard ||
        discipline == ActiveDiscipline::Bmx;
    intent.grind = rideDiscipline && frame.action(Action::Grind).held;
    intent.balance = rideDiscipline && frame.action(Action::Balance).held;
    intent.spinLeft = rideDiscipline && frame.action(Action::SpinLeft).held;
    intent.spinRight = rideDiscipline && frame.action(Action::SpinRight).held;
    return intent;
}

InputFrame InputResolver::resolve(
    const PhysicalInputFrame& physical
) const noexcept
{
    InputFrame frame;
    frame.activeDevice = physical.activeDevice;
    frame.controllerLayout = physical.controllerLayout;
    frame.gamepadAvailable = physical.gamepadAvailable;
    frame.gamepadConnected = physical.gamepadConnected;
    frame.gamepadDisconnected = physical.gamepadDisconnected;

    for (const ActionBinding& binding : kActionBindings) {
        ActionState& state = frame.actions[indexOf(binding.action)];
        const float value = std::clamp(
            std::fabs(physical.value(binding.control)),
            0.0f,
            1.0f
        );
        state.value = std::max(state.value, value);
        state.held = state.held || value > 0.20f;
        state.pressed = state.pressed || physical.wasPressed(binding.control);
    }

    for (const AxisBinding& binding : kAxisBindings) {
        frame.axes[indexOf(binding.axis)] +=
            physical.value(binding.control) * binding.scale;
    }
    frame.axes[indexOf(Axis::MoveRight)] = std::clamp(
        frame.axes[indexOf(Axis::MoveRight)], -1.0f, 1.0f
    );
    frame.axes[indexOf(Axis::MoveForward)] = std::clamp(
        frame.axes[indexOf(Axis::MoveForward)], -1.0f, 1.0f
    );
    frame.axes[indexOf(Axis::Accelerate)] = std::clamp(
        frame.axes[indexOf(Axis::Accelerate)], 0.0f, 1.0f
    );
    frame.axes[indexOf(Axis::RightStickX)] = std::clamp(
        frame.axes[indexOf(Axis::RightStickX)], -1.0f, 1.0f
    );
    frame.axes[indexOf(Axis::RightStickY)] = std::clamp(
        frame.axes[indexOf(Axis::RightStickY)], -1.0f, 1.0f
    );
    return frame;
}

std::string_view InputResolver::actionName(Action action) noexcept
{
    switch (action) {
        case Action::Jump: return "JUMP";
        case Action::Interact: return "INTERACT";
        case Action::Cancel: return "CANCEL";
        case Action::PrimaryAction: return "PRIMARY ACTION";
        case Action::SecondaryAction: return "SECONDARY ACTION";
        case Action::Guard: return "GUARD";
        case Action::Context: return "CONTEXT";
        case Action::Grind: return "GRIND";
        case Action::Balance: return "BALANCE";
        case Action::Accelerate: return "ACCELERATE";
        case Action::SpinLeft: return "SPIN LEFT";
        case Action::SpinRight: return "SPIN RIGHT";
        case Action::Dismount: return "DISMOUNT";
        case Action::Recover: return "RECOVER";
        case Action::Pause: return "PAUSE";
        case Action::Quit: return "QUIT";
        case Action::OrbitCamera: return "ORBIT";
        case Action::CameraReset: return "RESET CAMERA";
        case Action::SelectOnFoot: return "ON FOOT";
        case Action::SelectSkateboard: return "SKATEBOARD";
        case Action::SelectBmx: return "BMX";
        case Action::SelectCar: return "CAR";
        case Action::LookSlower: return "LOOK SLOWER";
        case Action::LookFaster: return "LOOK FASTER";
        case Action::VolumeDown: return "VOLUME DOWN";
        case Action::VolumeUp: return "VOLUME UP";
        case Action::ToggleRideFallback: return "DEV RIDE FALLBACK";
        case Action::CaptureExpertSnapshot: return "CAPTURE EXPERT SNAPSHOT";
        case Action::Count: break;
    }
    return "ACTION";
}

std::string_view InputResolver::prompt(
    Action action,
    InputDevice device,
    ControllerLayout layout
) noexcept
{
    if (device == InputDevice::Gamepad) {
        const auto face = [layout](std::string_view generic,
                                   std::string_view xbox,
                                   std::string_view playStation) {
            switch (layout) {
                case ControllerLayout::Xbox: return xbox;
                case ControllerLayout::PlayStation: return playStation;
                case ControllerLayout::Generic: return generic;
            }
            return generic;
        };
        switch (action) {
            case Action::Jump: return face("SOUTH", "A", "CROSS");
            case Action::Interact:
            case Action::PrimaryAction: return face("WEST", "X", "SQUARE");
            case Action::Cancel:
            case Action::Dismount: return face("EAST", "B", "CIRCLE");
            case Action::SecondaryAction:
            case Action::Grind: return face("NORTH", "Y", "TRIANGLE");
            case Action::Guard:
            case Action::Balance: return face("LEFT TRIGGER", "LT", "L2");
            case Action::SpinLeft: return face("LEFT SHOULDER", "LB", "L1");
            case Action::SpinRight: return face("RIGHT SHOULDER", "RB", "R1");
            case Action::Accelerate: return face("RIGHT TRIGGER", "RT", "R2");
            case Action::Pause: return face("START", "MENU", "OPTIONS");
            case Action::CameraReset: return "R3";
            case Action::Recover: return "R3";
            case Action::SelectOnFoot: return "DPAD UP";
            case Action::SelectSkateboard: return "DPAD LEFT";
            case Action::SelectBmx: return "DPAD RIGHT";
            default: return "--";
        }
    }

    switch (action) {
        case Action::Jump: return "SPACE";
        case Action::Interact: return "E";
        case Action::Cancel:
        case Action::Dismount: return "C";
        case Action::PrimaryAction: return "Z*";
        case Action::SecondaryAction: return "X*";
        case Action::Guard: return "Q";
        case Action::Context:
        case Action::Grind: return "F";
        case Action::Balance: return "LCTRL";
        case Action::Accelerate: return "LSHIFT";
        case Action::SpinLeft:
        case Action::SpinRight: return "--";
        case Action::Recover:
        case Action::CameraReset: return "R";
        case Action::Pause: return "ESC";
        case Action::Quit: return "F10";
        case Action::OrbitCamera: return "RMB";
        case Action::SelectOnFoot: return "1*";
        case Action::SelectSkateboard: return "2*";
        case Action::SelectBmx: return "3*";
        case Action::SelectCar: return "4*";
        case Action::ToggleRideFallback: return "F9*";
        case Action::CaptureExpertSnapshot: return "F12*";
        case Action::LookSlower: return "[";
        case Action::LookFaster: return "]";
        case Action::VolumeDown: return "-";
        case Action::VolumeUp: return "+";
        case Action::Count: break;
    }
    return "--";
}

std::string_view InputResolver::deviceName(InputDevice device) noexcept
{
    return device == InputDevice::Gamepad ? "GAMEPAD" : "KEYBOARD + MOUSE";
}

std::string_view InputResolver::controllerLayoutName(
    ControllerLayout layout
) noexcept
{
    switch (layout) {
        case ControllerLayout::Generic: return "GENERIC SDL";
        case ControllerLayout::Xbox: return "XBOX-STYLE";
        case ControllerLayout::PlayStation: return "PLAYSTATION-STYLE";
    }
    return "GENERIC SDL";
}

void DeviceActivityTracker::noteActivity(InputDevice device) noexcept
{
    if (device != InputDevice::Gamepad || gamepadAvailable_) {
        activeDevice_ = device;
    }
}

void DeviceActivityTracker::setGamepadAvailable(bool available) noexcept
{
    gamepadAvailable_ = available;
    if (!available && activeDevice_ == InputDevice::Gamepad) {
        activeDevice_ = InputDevice::KeyboardMouse;
    }
}

InputDevice DeviceActivityTracker::activeDevice() const noexcept
{
    return activeDevice_;
}

bool DeviceActivityTracker::gamepadAvailable() const noexcept
{
    return gamepadAvailable_;
}

} // namespace hakui::input
