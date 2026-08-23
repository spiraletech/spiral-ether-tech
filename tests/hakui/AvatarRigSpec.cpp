#include <cassert>
#include <cmath>
#include <cstddef>
#include <string_view>
#include <unordered_set>

#include "avatar/HakuiSkeleton.hpp"
#include "avatar/AvatarGroundContact.hpp"
#include "avatar/RideAttachmentRig.hpp"

namespace {

void default_humanoid_is_complete_and_rebuildable()
{
    HakuiSkeleton skeleton;

    assert(!skeleton.ready());
    assert(skeleton.buildDefaultHumanoid());
    assert(skeleton.ready());
    assert(skeleton.boneCount() == 23);

    const auto firstBoneCount = skeleton.boneCount();
    const auto firstSlotCount = skeleton.attachmentSlots().size();

    assert(skeleton.buildDefaultHumanoid());
    assert(skeleton.boneCount() == firstBoneCount);
    assert(skeleton.attachmentSlots().size() == firstSlotCount);
}

void hierarchy_and_attachment_references_are_valid()
{
    HakuiSkeleton skeleton;
    assert(skeleton.buildDefaultHumanoid());

    const auto& bones = skeleton.bones();
    assert(!bones.empty());
    assert(bones.front().name == "Root");
    assert(bones.front().parent == -1);

    std::unordered_set<std::string_view> names;
    for (std::size_t index = 0; index < bones.size(); ++index) {
        const auto& bone = bones[index];
        assert(!bone.name.empty());
        assert(names.insert(bone.name).second);

        if (index > 0) {
            assert(bone.parent >= 0);
            assert(static_cast<std::size_t>(bone.parent) < index);
        }
    }

    std::unordered_set<std::string_view> slots;
    for (const auto& attachment : skeleton.attachmentSlots()) {
        assert(!attachment.slot.empty());
        assert(slots.insert(attachment.slot).second);
        assert(skeleton.findBone(attachment.bone) >= 0);
    }

    assert(skeleton.findBone("Head") >= 0);
    assert(skeleton.findBone("missing") == -1);
}

void rideable_anchors_use_a_stable_forward_convention()
{
    using namespace hakui;

    const RideAttachmentRig board = RideAttachmentRig::skateboard();
    const RideAttachmentRig bmx = RideAttachmentRig::bmx();
    assert(board.valid());
    assert(bmx.valid());
    assert(board.find(RideAnchorSemantic::BoardDeckAnchor) != nullptr);

    const RideAnchor* front = bmx.find(RideAnchorSemantic::FrontAxle);
    const RideAnchor* rear = bmx.find(RideAnchorSemantic::RearAxle);
    const RideAnchor* pelvis = bmx.find(RideAnchorSemantic::PelvisAnchor);
    const RideAnchor* leftGrip = bmx.find(RideAnchorSemantic::LeftHandGrip);
    const RideAnchor* rightGrip = bmx.find(RideAnchorSemantic::RightHandGrip);
    assert(front && rear && pelvis && leftGrip && rightGrip);
    assert(front->z > rear->z);
    assert(leftGrip->z > pelvis->z);
    assert(rightGrip->z > pelvis->z);
    assert(leftGrip->x < 0.0f);
    assert(rightGrip->x > 0.0f);
    assert(leftGrip->y == rightGrip->y);
    assert(leftGrip->z == rightGrip->z);
}

void embodiment_profiles_agree_with_authored_contact_surfaces()
{
    using namespace hakui;
    constexpr EmbodimentProfileId profiles[] = {
        EmbodimentProfileId::OnFoot,
        EmbodimentProfileId::Skateboard,
        EmbodimentProfileId::Bmx,
        EmbodimentProfileId::Seated,
        EmbodimentProfileId::Combat,
        EmbodimentProfileId::Knockdown
    };
    for (const EmbodimentProfileId id : profiles) {
        const AvatarGroundContactProfile& profile = avatarGroundContactProfile(id);
        assert(profile.id == id);
        assert(!embodimentProfileName(id).empty());
        assert(std::fabs(standingFootContactError(profile)) < 0.0001f);
    }
    assert(avatarGroundContactProfile(EmbodimentProfileId::OnFoot)
               .feetConstrainGroundContact);
    assert(avatarGroundContactProfile(EmbodimentProfileId::Skateboard)
               .contact == GroundContactKind::BoardDeck);
    assert(!avatarGroundContactProfile(EmbodimentProfileId::Bmx)
                .feetConstrainGroundContact);
}

} // namespace

int main()
{
    default_humanoid_is_complete_and_rebuildable();
    hierarchy_and_attachment_references_are_valid();
    rideable_anchors_use_a_stable_forward_convention();
    embodiment_profiles_agree_with_authored_contact_surfaces();
    return 0;
}
