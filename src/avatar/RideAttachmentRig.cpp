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
        {RideAnchorSemantic::BoardRoot, 0.0f, 0.0f, 0.0f},
        {RideAnchorSemantic::RiderRoot, 0.0f, 0.0f, 0.0f,
         RideAnchorSemantic::BoardRoot},
        {RideAnchorSemantic::PelvisAnchor, 0.0f, 1.86f, 0.0f,
         RideAnchorSemantic::RiderRoot},
        {RideAnchorSemantic::DeckCenter, 0.0f, 0.18f, 0.0f,
         RideAnchorSemantic::BoardRoot},
        {RideAnchorSemantic::BoardDeckAnchor, 0.0f, 0.0f, 0.0f,
         RideAnchorSemantic::DeckCenter},
        {RideAnchorSemantic::FrontFootAnchor, -0.23f, 0.30f, 0.36f,
         RideAnchorSemantic::BoardRoot},
        {RideAnchorSemantic::RearFootAnchor, 0.23f, 0.30f, -0.36f,
         RideAnchorSemantic::BoardRoot},
        {RideAnchorSemantic::LeftFootAnchor, 0.0f, 0.0f, 0.0f,
         RideAnchorSemantic::FrontFootAnchor},
        {RideAnchorSemantic::RightFootAnchor, 0.0f, 0.0f, 0.0f,
         RideAnchorSemantic::RearFootAnchor},
        {RideAnchorSemantic::FrontTruck, 0.0f, 0.10f, 0.56f,
         RideAnchorSemantic::BoardRoot},
        {RideAnchorSemantic::RearTruck, 0.0f, 0.10f, -0.56f,
         RideAnchorSemantic::BoardRoot},
        {RideAnchorSemantic::FrontAxle, 0.0f, 0.0f, 0.0f,
         RideAnchorSemantic::FrontTruck},
        {RideAnchorSemantic::RearAxle, 0.0f, 0.0f, 0.0f,
         RideAnchorSemantic::RearTruck}
    });
}

RideAttachmentRig RideAttachmentRig::bmx()
{
    return RideAttachmentRig({
        {RideAnchorSemantic::BikeRoot, 0.0f, 0.0f, 0.0f},
        {RideAnchorSemantic::RiderRoot, 0.0f, 0.0f, 0.0f,
         RideAnchorSemantic::BikeRoot},
        {RideAnchorSemantic::PelvisAnchor, 0.0f, 1.62f, -0.22f,
         RideAnchorSemantic::RiderRoot},
        {RideAnchorSemantic::SeatAnchor, 0.0f, 1.36f, -0.32f,
         RideAnchorSemantic::BikeRoot},
        {RideAnchorSemantic::RearAxle, 0.0f, 0.65f, -0.88f,
         RideAnchorSemantic::BikeRoot},
        {RideAnchorSemantic::FrontSteeringAssembly, 0.0f, 0.65f, 0.88f,
         RideAnchorSemantic::BikeRoot},
        {RideAnchorSemantic::FrontAxle, 0.0f, 0.0f, 0.0f,
         RideAnchorSemantic::FrontSteeringAssembly},
        {RideAnchorSemantic::FrontWheel, 0.0f, 0.0f, 0.0f,
         RideAnchorSemantic::FrontSteeringAssembly},
        {RideAnchorSemantic::Fork, 0.0f, 0.29f, -0.15f,
         RideAnchorSemantic::FrontSteeringAssembly},
        {RideAnchorSemantic::Stem, 0.0f, 0.68f, -0.23f,
         RideAnchorSemantic::FrontSteeringAssembly},
        {RideAnchorSemantic::Handlebar, 0.0f, 0.20f, 0.03f,
         RideAnchorSemantic::Stem},
        {RideAnchorSemantic::LeftHandGrip, -0.62f, 0.0f, 0.0f,
         RideAnchorSemantic::Handlebar},
        {RideAnchorSemantic::RightHandGrip, 0.62f, 0.0f, 0.0f,
         RideAnchorSemantic::Handlebar},
        {RideAnchorSemantic::Crank, 0.0f, 0.66f, 0.05f,
         RideAnchorSemantic::BikeRoot},
        {RideAnchorSemantic::LeftPedal, -0.28f, 0.12f, 0.0f,
         RideAnchorSemantic::Crank},
        {RideAnchorSemantic::RightPedal, 0.28f, -0.12f, 0.0f,
         RideAnchorSemantic::Crank},
        {RideAnchorSemantic::LeftFootAnchor, 0.0f, 0.0f, 0.0f,
         RideAnchorSemantic::LeftPedal},
        {RideAnchorSemantic::RightFootAnchor, 0.0f, 0.0f, 0.0f,
         RideAnchorSemantic::RightPedal}
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

RideAnchorPosition RideAttachmentRig::unrotatedPosition(
    RideAnchorSemantic semantic,
    std::uint8_t depth
) const noexcept
{
    if (depth >= static_cast<std::uint8_t>(RideAnchorSemantic::Count)) {
        return {};
    }
    const RideAnchor* anchor = find(semantic);
    if (!anchor) {
        return {};
    }
    const RideAnchorPosition parent = anchor->parent == RideAnchorSemantic::None
        ? RideAnchorPosition{}
        : unrotatedPosition(anchor->parent, static_cast<std::uint8_t>(depth + 1));
    return {parent.x + anchor->x, parent.y + anchor->y, parent.z + anchor->z};
}

bool RideAttachmentRig::descendsFrom(
    RideAnchorSemantic semantic,
    RideAnchorSemantic ancestor,
    std::uint8_t depth
) const noexcept
{
    if (semantic == ancestor) {
        return true;
    }
    if (depth >= static_cast<std::uint8_t>(RideAnchorSemantic::Count)) {
        return false;
    }
    const RideAnchor* anchor = find(semantic);
    return anchor && anchor->parent != RideAnchorSemantic::None &&
        descendsFrom(
            anchor->parent,
            ancestor,
            static_cast<std::uint8_t>(depth + 1)
        );
}

RideAnchorPosition RideAttachmentRig::resolvePosition(
    RideAnchorSemantic semantic,
    float steeringYaw,
    float crankRotation
) const noexcept
{
    RideAnchorPosition result = unrotatedPosition(semantic);
    if (descendsFrom(semantic, RideAnchorSemantic::FrontSteeringAssembly)) {
        const RideAnchorPosition pivot = unrotatedPosition(
            RideAnchorSemantic::FrontSteeringAssembly
        );
        const float localX = result.x - pivot.x;
        const float localZ = result.z - pivot.z;
        const float cosine = std::cos(steeringYaw);
        const float sine = std::sin(steeringYaw);
        result.x = pivot.x + localX * cosine + localZ * sine;
        result.z = pivot.z - localX * sine + localZ * cosine;
    }
    const bool pedalDescendant =
        descendsFrom(semantic, RideAnchorSemantic::LeftPedal) ||
        descendsFrom(semantic, RideAnchorSemantic::RightPedal);
    if (pedalDescendant) {
        const RideAnchorPosition pivot = unrotatedPosition(RideAnchorSemantic::Crank);
        const float localY = result.y - pivot.y;
        const float localZ = result.z - pivot.z;
        const float cosine = std::cos(crankRotation);
        const float sine = std::sin(crankRotation);
        result.y = pivot.y + localY * cosine - localZ * sine;
        result.z = pivot.z + localY * sine + localZ * cosine;
    }
    return result;
}

std::span<const RideAnchor> RideAttachmentRig::anchors() const noexcept
{
    return anchors_;
}

bool RideAttachmentRig::valid() const noexcept
{
    std::array<bool, static_cast<std::size_t>(RideAnchorSemantic::Count)> found{};
    for (const RideAnchor& anchor : anchors_) {
        const std::size_t index = static_cast<std::size_t>(anchor.semantic);
        if (index >= found.size() || found[index] ||
            !std::isfinite(anchor.x) || !std::isfinite(anchor.y) ||
            !std::isfinite(anchor.z)) {
            return false;
        }
        found[index] = true;
    }
    for (const RideAnchor& anchor : anchors_) {
        if (anchor.parent != RideAnchorSemantic::None &&
            (anchor.parent == anchor.semantic || !find(anchor.parent))) {
            return false;
        }
        std::array<bool, static_cast<std::size_t>(RideAnchorSemantic::Count)>
            visited{};
        RideAnchorSemantic cursor = anchor.semantic;
        while (cursor != RideAnchorSemantic::None) {
            const std::size_t cursorIndex = static_cast<std::size_t>(cursor);
            if (cursorIndex >= visited.size() || visited[cursorIndex]) {
                return false;
            }
            visited[cursorIndex] = true;
            const RideAnchor* current = find(cursor);
            cursor = current ? current->parent : RideAnchorSemantic::None;
        }
        const RideAnchorPosition resolved = unrotatedPosition(anchor.semantic);
        if (!std::isfinite(resolved.x) || !std::isfinite(resolved.y) ||
            !std::isfinite(resolved.z)) {
            return false;
        }
    }
    const bool base =
        found[static_cast<std::size_t>(RideAnchorSemantic::RiderRoot)] &&
        found[static_cast<std::size_t>(RideAnchorSemantic::PelvisAnchor)] &&
        found[static_cast<std::size_t>(RideAnchorSemantic::LeftFootAnchor)] &&
        found[static_cast<std::size_t>(RideAnchorSemantic::RightFootAnchor)];
    const bool board = found[static_cast<std::size_t>(RideAnchorSemantic::BoardRoot)];
    const bool bike = found[static_cast<std::size_t>(RideAnchorSemantic::BikeRoot)];
    return base && ((board &&
        found[static_cast<std::size_t>(RideAnchorSemantic::FrontFootAnchor)] &&
        found[static_cast<std::size_t>(RideAnchorSemantic::RearFootAnchor)] &&
        found[static_cast<std::size_t>(RideAnchorSemantic::DeckCenter)]) ||
        (bike &&
        found[static_cast<std::size_t>(RideAnchorSemantic::FrontSteeringAssembly)] &&
        found[static_cast<std::size_t>(RideAnchorSemantic::LeftHandGrip)] &&
        found[static_cast<std::size_t>(RideAnchorSemantic::RightHandGrip)] &&
        found[static_cast<std::size_t>(RideAnchorSemantic::LeftPedal)] &&
        found[static_cast<std::size_t>(RideAnchorSemantic::RightPedal)]));
}

} // namespace hakui
