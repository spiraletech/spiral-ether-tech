#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace hakui {

// Every rideable uses +Z as local forward, +Y as up, and +X as rider-right.
// These semantics are stable seams for later hand/foot IK; they are not bone
// solver data yet.
enum class RideAnchorSemantic : std::uint8_t {
    RiderRoot,
    PelvisAnchor,
    LeftFootAnchor,
    RightFootAnchor,
    LeftHandGrip,
    RightHandGrip,
    FrontAxle,
    RearAxle,
    BoardDeckAnchor
};

struct RideAnchor {
    RideAnchorSemantic semantic = RideAnchorSemantic::RiderRoot;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

class RideAttachmentRig {
public:
    static RideAttachmentRig skateboard();
    static RideAttachmentRig bmx();

    const RideAnchor* find(RideAnchorSemantic semantic) const noexcept;
    std::span<const RideAnchor> anchors() const noexcept;
    bool valid() const noexcept;

private:
    explicit RideAttachmentRig(std::vector<RideAnchor> anchors);

    std::vector<RideAnchor> anchors_;
};

} // namespace hakui
