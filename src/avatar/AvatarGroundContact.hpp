#pragma once

#include <cstdint>
#include <string_view>

namespace hakui {

enum class EmbodimentProfileId : std::uint8_t {
    OnFoot,
    Skateboard,
    Bmx,
    Seated,
    Combat,
    Knockdown
};

enum class GroundContactKind : std::uint8_t {
    FootSoles,
    BoardDeck,
    Wheels,
    SeatAnchor,
    BodyContact
};

struct AvatarGroundContactProfile {
    EmbodimentProfileId id = EmbodimentProfileId::OnFoot;
    GroundContactKind contact = GroundContactKind::FootSoles;
    float visualRootAbovePlayerBase = 0.43f;
    float contactSurfaceAbovePlayerBase = 0.0f;
    float standingFootSoleBelowVisualRoot = 0.43f;
    bool feetConstrainGroundContact = true;
};

const AvatarGroundContactProfile& avatarGroundContactProfile(
    EmbodimentProfileId id
) noexcept;
std::string_view embodimentProfileName(EmbodimentProfileId id) noexcept;
float standingFootContactError(const AvatarGroundContactProfile& profile) noexcept;

} // namespace hakui
