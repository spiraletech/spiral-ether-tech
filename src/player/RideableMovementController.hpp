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
    PopShoveIt,
    Impossible,
    VarialFlip,
    BoardGrab,
    BmxTabletop,
    BmxTailwhipLeft,
    BmxTailwhipRight,
    BmxBarspin,
    BmxCrankflip,
    BmxXUp,
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
    Clean,
    Sketchy,
    Failed,
    Bail
};

enum class BailReason : std::uint8_t {
    None,
    UnderRotated,
    InvertedRideable,
    ExcessiveImpact,
    ExcessiveAngularVelocity,
    ContactMisalignment,
    LostBalance
};

enum class RideRotationChannel : std::uint8_t {
    None,
    Rideable,
    BoardDeck,
    BmxFrame,
    BmxSteering,
    BmxCrank
};

enum class SkateStance : std::uint8_t {
    Regular,
    Goofy
};

enum class RideFootContactState : std::uint8_t {
    Anchored,
    ReleasedForTrick,
    Reacquiring,
    Landed
};

enum class RideAirPose : std::uint8_t {
    None,
    OlliePop,
    OllieRise,
    OllieLevel,
    OllieDescent,
    Kickflip,
    Heelflip,
    PopShoveIt,
    Impossible,
    VarialFlip,
    BoardGrab,
    BmxPull,
    BmxTuck,
    BmxDescent,
    BmxTrick,
    Bail
};

struct RideRotation {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct TrickPhysicalIntent {
    RideRotationChannel channel = RideRotationChannel::None;
    RideRotation axis{};
    float direction = 1.0f;
    float rotationTarget = 0.0f;
    float angularSpeed = 0.0f;
    float minimumAirtime = 0.0f;
    float bodyRotationAssist = 0.0f;
    bool returnToNeutral = false;
};

struct RidePhysicsTuning {
    float skateboardPopImpulseMin = 6.25f;
    float skateboardPopImpulseMax = 8.20f;
    float bmxPopImpulseMin = 6.85f;
    float bmxPopImpulseMax = 8.80f;
    float cleanCompletion = 0.94f;
    float sketchyCompletion = 0.80f;
    float cleanImpactSpeed = 9.75f;
    float sketchyImpactSpeed = 12.25f;
    float cleanAngularSpeed = 1.20f;
    float sketchyAngularSpeed = 3.80f;
};

// Deterministic pose vocabulary. This is intentionally renderer-independent:
// the movement machine describes the body mechanics that explain its physical
// state, while SDL merely turns those values into a temporary procedural rig.
struct RideBodyMechanicsState {
    SkateStance skateStance = SkateStance::Regular;
    RideFootContactState footContact = RideFootContactState::Anchored;
    RideAirPose airPose = RideAirPose::None;
    float pelvisYawRelativeToBoard = 0.0f;
    float torsoYawRelativeToBoard = 0.0f;
    float headYawRelativeToBoard = 0.0f;
    float leftKneeFlex = 0.0f;
    float rightKneeFlex = 0.0f;
    float leftElbowFlex = 0.0f;
    float rightElbowFlex = 0.0f;
    float preloadPoseWeight = 0.0f;
    float landingCompression = 0.0f;
    float frontFootAnchorError = 0.0f;
    float rearFootAnchorError = 0.0f;
    float frontFootLift = 0.0f;
    float rearLegDrive = 0.0f;
    float torsoLean = 0.0f;
    float armCounterbalance = 0.0f;
};

struct RideableInput {
    MovementInput movement{};
    bool popPressed = false;
    float popPreload = 0.0f;
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
    float trickSeconds = 0.0f;
    float comboWindowSeconds = 0.0f;
    float steeringVisual = 0.0f;
    float bodySpinRadians = 0.0f;
    float spinVelocity = 0.0f;
    float propulsion = 0.0f;
    float popPreload = 0.0f;
    float popImpulse = 0.0f;
    RideRotation rideableRotation{};
    RideRotation angularVelocity{};
    RideRotation targetRotation{};
    RideRotation surfaceNormal{0.0f, 1.0f, 0.0f};
    float rotationCompletion = 1.0f;
    float rotationTravel = 0.0f;
    float rotationTravelTarget = 0.0f;
    float minimumTrickAirtime = 0.0f;
    float bodyRotationAssist = 0.0f;
    float footContactAlignment = 1.0f;
    float leftHandGripError = 0.0f;
    float rightHandGripError = 0.0f;
    float leftFootAnchorError = 0.0f;
    float rightFootAnchorError = 0.0f;
    float tumbleRadians = 0.0f;
    float rideSeparation = 0.0f;
    float rideSeparationVelocity = 0.0f;
    RideRotationChannel rotationChannel = RideRotationChannel::None;
    BailReason bailReason = BailReason::None;
    RideTrickDirection lastTrickDirection = RideTrickDirection::None;
    RideGrindAttachment activeGrindAttachment = RideGrindAttachment::None;
    std::uint32_t activeAffordanceId = 0;
    std::array<RideTrick, 8> combo{};
    std::size_t comboCount = 0;
    bool flipCommitted = false;
    bool rotationReturning = false;
    RideBodyMechanicsState body{};
};

struct RideableFrame {
    MovementStep movement{};
    bool trickStarted = false;
    bool grindStarted = false;
    bool manualStarted = false;
    bool landed = false;
    bool bailed = false;
    LandingQuality evaluatedLanding = LandingQuality::None;
};

// Deterministic expressive-movement layer. The ordinary movement controller
// continues to own collision, acceleration, gravity, and void recovery. This
// layer interprets semantic world affordances as skateboard/BMX opportunities.
class RideableMovementController {
public:
    RideableMovementController() = default;
    explicit RideableMovementController(RidePhysicsTuning tuning);

    RideableFrame update(
        PlayerState& player,
        const RideableInput& input,
        const MovementEnvironment& environment,
        std::span<const WorldAffordanceVolume> affordances,
        float deltaSeconds
    ) noexcept;

    void reset() noexcept;
    void setSkateStance(SkateStance stance) noexcept;
    const RideableState& state() const noexcept;
    const RidePhysicsTuning& tuning() const noexcept;

    static std::string_view phaseLabel(RidePhase phase) noexcept;
    static std::string_view trickLabel(RideTrick trick) noexcept;
    static std::string_view landingLabel(LandingQuality quality) noexcept;
    static std::string_view bailReasonLabel(BailReason reason) noexcept;
    static std::string_view rotationChannelLabel(
        RideRotationChannel channel
    ) noexcept;
    static std::string_view skateStanceLabel(SkateStance stance) noexcept;
    static std::string_view footContactLabel(
        RideFootContactState state
    ) noexcept;
    static std::string_view airPoseLabel(RideAirPose pose) noexcept;
    static TrickPhysicalIntent physicalIntentFor(RideTrick trick) noexcept;

private:
    const WorldAffordanceVolume* nearby(
        WorldAffordance affordance,
        const PlayerState& player,
        std::span<const WorldAffordanceVolume> volumes,
        float padding
    ) const noexcept;
    void beginTrick(RideTrick trick, RideableFrame& frame) noexcept;
    void appendCombo(RideTrick trick) noexcept;
    void beginBail(
        PlayerState& player,
        RideableFrame& frame,
        BailReason reason,
        LandingQuality quality = LandingQuality::Bail
    ) noexcept;
    void beginPhysicalTrick(RideTrick trick, RideableFrame& frame) noexcept;
    void updatePhysicalRotation(float deltaSeconds) noexcept;
    void updateBodyMechanics(const PlayerState& player) noexcept;
    LandingQuality evaluateLanding(
        float verticalImpactSpeed,
        float horizontalSpeed,
        const RideRotation& surfaceNormal,
        BailReason& reason
    ) const noexcept;
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
    RidePhysicsTuning tuning_{};
    RideableState state_{};
    SkateStance skateStance_ = SkateStance::Regular;
};

} // namespace hakui
