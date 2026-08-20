#include <cassert>
#include <cstddef>
#include <string_view>
#include <unordered_set>

#include "avatar/HakuiSkeleton.hpp"

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

} // namespace

int main()
{
    default_humanoid_is_complete_and_rebuildable();
    hierarchy_and_attachment_references_are_valid();
    return 0;
}
