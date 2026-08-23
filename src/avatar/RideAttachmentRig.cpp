#include "avatar/RideAttachmentRig.hpp"

#include <array>
#include <cmath>

namespace hakui {

RideAttachmentRig::RideAttachmentRig(std::vector<RideAnchor> anchors)
    : anchors_(std::move(anchors))
{
}

RideAttachmentRig RideAttachmentRig::skateboard()
{
    return RideAttachmentRig({
        {RideAnchorSemantic::RiderRoot, 0.0f, 0.0f, 0.0f},
        {RideAnchorSemantic::PelvisAnchor, 0.0f, 1.40f, 0.0f},
        {RideAnchorSemantic::LeftFootAnchor, -0.23f, 0.30f, -0.36f},
        {RideAnchorSemantic::RightFootAnchor, 0.23f, 0.30f, 0.36f},
        {RideAnchorSemantic::FrontAxle, 0.0f, 0.10f, 0.56f},
        {RideAnchorSemantic::RearAxle, 0.0f, 0.10f, -0.56f},
        {RideAnchorSemantic::BoardDeckAnchor, 0.0f, 0.18f, 0.0f}
    });
}

RideAttachmentRig RideAttachmentRig::bmx()
{
    return RideAttachmentRig({
        {RideAnchorSemantic::RiderRoot, 0.0f, 0.0f, 0.0f},
        {RideAnchorSemantic::PelvisAnchor, 0.0f, 1.62f, -0.22f},
        {RideAnchorSemantic::LeftFootAnchor, -0.28f, 0.66f, 0.05f},
        {RideAnchorSemantic::RightFootAnchor, 0.28f, 0.66f, 0.05f},
        {RideAnchorSemantic::LeftHandGrip, -0.62f, 1.53f, 0.68f},
        {RideAnchorSemantic::RightHandGrip, 0.62f, 1.53f, 0.68f},
        {RideAnchorSemantic::FrontAxle, 0.0f, 0.65f, 0.88f},
        {RideAnchorSemantic::RearAxle, 0.0f, 0.65f, -0.88f}
    });
}

const RideAnchor* RideAttachmentRig::find(
    RideAnchorSemantic semantic
) const noexcept
{
    for (const RideAnchor& anchor : anchors_) {
        if (anchor.semantic == semantic) {
            return &anchor;
        }
    }
    return nullptr;
}

std::span<const RideAnchor> RideAttachmentRig::anchors() const noexcept
{
    return anchors_;
}

bool RideAttachmentRig::valid() const noexcept
{
    std::array<bool, 9> found{};
    for (const RideAnchor& anchor : anchors_) {
        const std::size_t index = static_cast<std::size_t>(anchor.semantic);
        if (index >= found.size() || found[index] ||
            !std::isfinite(anchor.x) || !std::isfinite(anchor.y) ||
            !std::isfinite(anchor.z)) {
            return false;
        }
        found[index] = true;
    }
    return found[static_cast<std::size_t>(RideAnchorSemantic::RiderRoot)] &&
        found[static_cast<std::size_t>(RideAnchorSemantic::PelvisAnchor)] &&
        found[static_cast<std::size_t>(RideAnchorSemantic::LeftFootAnchor)] &&
        found[static_cast<std::size_t>(RideAnchorSemantic::RightFootAnchor)];
}

} // namespace hakui
