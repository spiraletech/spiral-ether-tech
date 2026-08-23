#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace hakui {

// Every rideable uses +Z as local forward, +Y as up, and +X as rider-right.
// These semantics are stable seams for later hand/foot IK; they are not bone
// solver data yet.
enum class RideAnchorSemantic : std::uint8_t {
    None,
    BikeRoot,
    BoardRoot,
    RiderRoot,
    PelvisAnchor,
    LeftFootAnchor,
    RightFootAnchor,
    LeftHandGrip,
    RightHandGrip,
    FrontSteeringAssembly,
    Fork,
    FrontWheel,
    Stem,
    Handlebar,
    FrontAxle,
    RearAxle,
    Crank,
    LeftPedal,
    RightPedal,
    SeatAnchor,
    BoardDeckAnchor,
    FrontFootAnchor,
    RearFootAnchor,
    FrontTruck,
    RearTruck,
    DeckCenter,
    Count
};

struct RideAnchor {
    RideAnchorSemantic semantic = RideAnchorSemantic::RiderRoot;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    RideAnchorSemantic parent = RideAnchorSemantic::None;
};

struct RideAnchorPosition {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

class RideAttachmentRig {
public:
    static RideAttachmentRig skateboard();
    static RideAttachmentRig bmx();

    const RideAnchor* find(RideAnchorSemantic semantic) const noexcept;
    RideAnchorPosition resolvePosition(
        RideAnchorSemantic semantic,
        float steeringYaw = 0.0f,
        float crankRotation = 0.0f
    ) const noexcept;
    std::span<const RideAnchor> anchors() const noexcept;
    bool valid() const noexcept;

private:
    explicit RideAttachmentRig(std::vector<RideAnchor> anchors);

    RideAnchorPosition unrotatedPosition(
        RideAnchorSemantic semantic,
        std::uint8_t depth = 0
    ) const noexcept;
    bool descendsFrom(
        RideAnchorSemantic semantic,
        RideAnchorSemantic ancestor,
        std::uint8_t depth = 0
    ) const noexcept;

    std::vector<RideAnchor> anchors_;
};

} // namespace hakui
