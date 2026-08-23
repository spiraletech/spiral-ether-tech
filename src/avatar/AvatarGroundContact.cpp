#include "avatar/AvatarGroundContact.hpp"

#include <array>
#include <cstddef>

namespace hakui {

namespace {

constexpr std::array<AvatarGroundContactProfile, 6> kProfiles{{
    {EmbodimentProfileId::OnFoot, GroundContactKind::FootSoles,
     0.43f, 0.0f, 0.43f, true},
    {EmbodimentProfileId::Skateboard, GroundContactKind::BoardDeck,
     0.66f, 0.23f, 0.43f, true},
    {EmbodimentProfileId::Bmx, GroundContactKind::Wheels,
     0.42f, 0.0f, 0.43f, false},
    {EmbodimentProfileId::Seated, GroundContactKind::SeatAnchor,
     0.0f, 0.0f, 0.43f, false},
    {EmbodimentProfileId::Combat, GroundContactKind::FootSoles,
     0.43f, 0.0f, 0.43f, true},
    {EmbodimentProfileId::Knockdown, GroundContactKind::BodyContact,
     0.34f, 0.0f, 0.43f, false}
}};

} // namespace

const AvatarGroundContactProfile& avatarGroundContactProfile(
    EmbodimentProfileId id
) noexcept
{
    return kProfiles[static_cast<std::size_t>(id)];
}

std::string_view embodimentProfileName(EmbodimentProfileId id) noexcept
{
    switch (id) {
        case EmbodimentProfileId::OnFoot: return "on_foot";
        case EmbodimentProfileId::Skateboard: return "skateboard";
        case EmbodimentProfileId::Bmx: return "bmx";
        case EmbodimentProfileId::Seated: return "seated";
        case EmbodimentProfileId::Combat: return "combat";
        case EmbodimentProfileId::Knockdown: return "knockdown";
    }
    return "unknown";
}

float standingFootContactError(
    const AvatarGroundContactProfile& profile
) noexcept
{
    if (!profile.feetConstrainGroundContact) {
        return 0.0f;
    }
    return profile.visualRootAbovePlayerBase -
        profile.standingFootSoleBelowVisualRoot -
        profile.contactSurfaceAbovePlayerBase;
}

} // namespace hakui
