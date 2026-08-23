#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

#include "player/PlayerMovementController.hpp"
#include "world/WorldGeometry.hpp"

namespace hakui {

enum class RideDiscipline : std::uint8_t {
    None,
    Skateboard,
    BMX
};

enum class RidePhase : std::uint8_t {
    Grounded,
    Airborne,
    Grinding,
    Manual,
    Landing,
    Crash
};

enum class RideGrindAttachment : std::uint8_t {
    None,
    BoardTrucks,
    BmxPegs
};

enum class RideTrick : std::uint8_t {
    None,
    Ollie,
    BunnyHop,
    Kickflip,
    Heelflip,
    PopShove,
    VarialFlip,
    BoardGrab,
    BmxTabletop,
    BmxTailwhipLeft,
    BmxTailwhipRight,
    BmxBarspin,
    BmxCrankflip,
    BoardGrind,
    PegGrind,
    BoardManual,
    WheelManual,
    Land,
    Bail
};

enum class RideTrickDirection : std::uint8_t {
    None,
    Left,
    Right,
    Up,
    Down,
    UpLeft,
    UpRight,
    DownLeft,
    DownRight
};

struct RideTrickIntent {
    RideTrickDirection direction = RideTrickDirection::None;
    float x = 0.0f;
    float y = 0.0f;
    float magnitude = 0.0f;
    float duration = 0.0f;
    bool valid = false;
};

enum class LandingQuality : std::uint8_t {
    None,
    Sketchy,
    Clean,
    Perfect,
    Bail
};

struct RideableInput {
    MovementInput movement{};
    bool popPressed = false;
    bool manualHeld = false;
    bool grindHeld = false;
    bool stylePressed = false;
    bool spinLeft = false;
    bool spinRight = false;
    float propulsion = 0.0f;
    RideTrickIntent trick{};
};

struct RideableState {
    RideDiscipline discipline = RideDiscipline::None;
    RidePhase phase = RidePhase::Grounded;
    RideTrick activeTrick = RideTrick::None;
    LandingQuality landingQuality = LandingQuality::None;
    float speed = 0.0f;
    float momentum = 0.0f;
    float balance = 100.0f;
    float balanceOffset = 0.0f;
    float phaseSeconds = 0.0f;
    float airSeconds = 0.0f;
    float comboWindowSeconds = 0.0f;
    float steeringVisual = 0.0f;
    float bodySpinRadians = 0.0f;
    float spinVelocity = 0.0f;
    float propulsion = 0.0f;
    RideTrickDirection lastTrickDirection = RideTrickDirection::None;
    RideGrindAttachment activeGrindAttachment = RideGrindAttachment::None;
    std::uint32_t activeAffordanceId = 0;
    std::array<RideTrick, 8> combo{};
    std::size_t comboCount = 0;
    bool flipCommitted = false;
};

struct RideableFrame {
    MovementStep movement{};
    bool trickStarted = false;
    bool grindStarted = false;
    bool manualStarted = false;
    bool landed = false;
    bool bailed = false;
};

// Deterministic expressive-movement layer. The ordinary movement controller
// continues to own collision, acceleration, gravity, and void recovery. This
// layer interprets semantic world affordances as skateboard/BMX opportunities.
class RideableMovementController {
public:
    RideableMovementController() = default;

    RideableFrame update(
        PlayerState& player,
        const RideableInput& input,
        const MovementEnvironment& environment,
        std::span<const WorldAffordanceVolume> affordances,
        float deltaSeconds
    ) noexcept;

    void reset() noexcept;
    const RideableState& state() const noexcept;

    static std::string_view phaseLabel(RidePhase phase) noexcept;
    static std::string_view trickLabel(RideTrick trick) noexcept;
    static std::string_view landingLabel(LandingQuality quality) noexcept;

private:
    const WorldAffordanceVolume* nearby(
        WorldAffordance affordance,
        const PlayerState& player,
        std::span<const WorldAffordanceVolume> volumes,
        float padding
    ) const noexcept;
    void beginTrick(RideTrick trick, RideableFrame& frame) noexcept;
    void appendCombo(RideTrick trick) noexcept;
    void beginBail(PlayerState& player, RideableFrame& frame) noexcept;
    void updateBalance(
        float correction,
        float naturalDrift,
        float deltaSeconds
    ) noexcept;
    bool validGrindApproach(
        const WorldAffordanceVolume& grind,
        const PlayerState& player
    ) const noexcept;
    static RideTrick trickFor(
        RideDiscipline discipline,
        RideTrickDirection direction
    ) noexcept;

    PlayerMovementController movement_{};
    RideableState state_{};
};

} // namespace hakui
