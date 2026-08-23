#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string_view>

namespace hakui::input {

enum class InputDevice : std::uint8_t {
    KeyboardMouse,
    Gamepad
};

enum class ControllerLayout : std::uint8_t {
    Generic,
    Xbox,
    PlayStation
};

// Platform-neutral physical controls. SDL scancodes, mouse events, and
// controller mappings terminate in the native adapter before reaching here.
enum class PhysicalControl : std::uint8_t {
    KeyMoveLeft,
    KeyMoveRight,
    KeyMoveForward,
    KeyMoveBackward,
    KeyJump,
    KeyAccelerate,
    KeyInteract,
    KeyGuard,
    KeyContext,
    KeyBalance,
    KeyCancel,
    KeyRecover,
    KeyPrimaryLegacy,
    KeySecondaryLegacy,
    KeyGuardLegacy,
    KeyBalanceLegacy,
    KeyPause,
    KeyQuit,
    KeyExpertSnapshot,
    KeyModeOnFoot,
    KeyModeSkateboard,
    KeyModeBmx,
    KeyModeCar,
    KeyLookSlower,
    KeyLookFaster,
    KeyVolumeDown,
    KeyVolumeUp,
    KeyToggleRideFallback,
    MouseOrbit,
    MouseLookX,
    MouseLookY,
    MouseWheel,
    PadMoveX,
    PadMoveY,
    PadLookX,
    PadLookY,
    PadLeftTrigger,
    PadRightTrigger,
    PadSouth,
    PadEast,
    PadWest,
    PadNorth,
    PadLeftShoulder,
    PadRightShoulder,
    PadLeftStick,
    PadRightStick,
    PadStart,
    PadDpadUp,
    PadDpadDown,
    PadDpadLeft,
    PadDpadRight,
    Count
};

enum class Action : std::uint8_t {
    Jump,
    Interact,
    Cancel,
    PrimaryAction,
    SecondaryAction,
    Guard,
    Context,
    Grind,
    Balance,
    Accelerate,
    SpinLeft,
    SpinRight,
    Dismount,
    Recover,
    Pause,
    Quit,
    OrbitCamera,
    CameraReset,
    SelectOnFoot,
    SelectSkateboard,
    SelectBmx,
    SelectCar,
    LookSlower,
    LookFaster,
    VolumeDown,
    VolumeUp,
    ToggleRideFallback,
    CaptureExpertSnapshot,
    Count
};

enum class Axis : std::uint8_t {
    MoveRight,
    MoveForward,
    LookRight,
    LookDown,
    RightStickX,
    RightStickY,
    Zoom,
    Accelerate,
    Count
};

enum class ActiveDiscipline : std::uint8_t {
    OnFoot,
    Skateboard,
    Bmx,
    Boxing,
    Seated
};

enum class TraversalIntent : std::uint8_t {
    None,
    Jump,
    Ollie,
    BunnyHop
};

constexpr std::size_t physicalControlCount =
    static_cast<std::size_t>(PhysicalControl::Count);
constexpr std::size_t actionCount = static_cast<std::size_t>(Action::Count);
constexpr std::size_t axisCount = static_cast<std::size_t>(Axis::Count);

struct PhysicalInputFrame {
    std::array<float, physicalControlCount> values{};
    std::array<bool, physicalControlCount> pressed{};
    InputDevice activeDevice = InputDevice::KeyboardMouse;
    ControllerLayout controllerLayout = ControllerLayout::Generic;
    bool gamepadAvailable = false;
    bool gamepadConnected = false;
    bool gamepadDisconnected = false;

    void set(
        PhysicalControl control,
        float value,
        bool wasPressed = false
    ) noexcept;
    float value(PhysicalControl control) const noexcept;
    bool wasPressed(PhysicalControl control) const noexcept;
};

struct ActionState {
    float value = 0.0f;
    bool held = false;
    bool pressed = false;
};

struct InputFrame {
    std::array<ActionState, actionCount> actions{};
    std::array<float, axisCount> axes{};
    InputDevice activeDevice = InputDevice::KeyboardMouse;
    ControllerLayout controllerLayout = ControllerLayout::Generic;
    bool gamepadAvailable = false;
    bool gamepadConnected = false;
    bool gamepadDisconnected = false;

    const ActionState& action(Action semantic) const noexcept;
    float axis(Axis semantic) const noexcept;
};

// A context interpretation of the shared action grammar. Hardware never
// reaches this layer: the same Jump action becomes a jump, ollie, or bunny hop
// according to the active embodiment discipline.
struct DisciplineIntent {
    float moveRight = 0.0f;
    float moveForward = 0.0f;
    float accelerate = 0.0f;
    TraversalIntent traversal = TraversalIntent::None;
    bool interact = false;
    bool cancel = false;
    bool primary = false;
    bool secondary = false;
    bool guard = false;
    bool grind = false;
    bool balance = false;
    bool spinLeft = false;
    bool spinRight = false;
    bool recover = false;
};

class DisciplineInterpreter {
public:
    static DisciplineIntent interpret(
        const InputFrame& frame,
        ActiveDiscipline discipline
    ) noexcept;
};

class InputResolver {
public:
    InputFrame resolve(const PhysicalInputFrame& physical) const noexcept;

    static std::string_view actionName(Action action) noexcept;
    static std::string_view prompt(
        Action action,
        InputDevice device,
        ControllerLayout layout = ControllerLayout::Generic
    ) noexcept;
    static std::string_view deviceName(InputDevice device) noexcept;
    static std::string_view controllerLayoutName(ControllerLayout layout) noexcept;
};

class DeviceActivityTracker {
public:
    void noteActivity(InputDevice device) noexcept;
    void setGamepadAvailable(bool available) noexcept;
    InputDevice activeDevice() const noexcept;
    bool gamepadAvailable() const noexcept;

private:
    InputDevice activeDevice_ = InputDevice::KeyboardMouse;
    bool gamepadAvailable_ = false;
};

} // namespace hakui::input
