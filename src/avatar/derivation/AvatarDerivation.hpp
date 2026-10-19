#pragma once

#include "avatar/import/GlbMeshLoader.hpp"

#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace hakui::avatar {

// An avatar is derived from an authored neutral anatomical mesh, not
// from renderer-specific cylinder/cube geometry.
enum class ShapeChannel : std::uint8_t {
    Stature, ShoulderWidth, ChestVolume, WaistWidth, HipWidth,
    ArmLength, LegLength, HeadSize, JawWidth, NoseShape,
    EyeShape, MouthShape, Count
};
inline constexpr std::size_t kShapeChannelCount =
    static_cast<std::size_t>(ShapeChannel::Count);

std::string_view shapeChannelName(ShapeChannel channel) noexcept;

struct AvatarProfile {
    std::string id = "NEUTRAL";
    // Each channel is -1 .. +1, with separately authored POS / NEG targets.
    std::array<float, kShapeChannelCount> shape{};
    // Keep world scale separate from the skinned mesh / inverse bind matrices.
    float uniformScale = 1.0f;
};
struct MaterialStyle {
    std::string materialName;
    std::array<float, 4> baseColor{1.f, 1.f, 1.f, 1.f};
};
struct AttachmentSpec {
    // HAIR, HEADWEAR, SHIRT_OUTER, SHIRT_UNDER, PANTS,
    // FOOTWEAR, ACCESSORY_NECK, HAND_LEFT, HAND_RIGHT, BOARD_BACK.
    std::string slot;
    std::string assetPath;
    std::string boneName; // empty = resolve canonical anchor for slot
    bool skinned = false; // indicates a separately authored compatible rig
};
struct ResolvedAttachment {
    AttachmentSpec spec;
    std::string anchorBone;
    int anchorJointIndex = -1;
};
struct DerivedAvatar {
    import::GlbMesh mesh;
    AvatarProfile profile;
    std::vector<MaterialStyle> materials;
    std::vector<ResolvedAttachment> attachments;
    std::vector<std::string> consumedShapeKeys;
};
struct DeriveResult {
    bool ok = false;
    std::string error;
    DerivedAvatar avatar;
};

// The source mesh is immutable. Every nonzero shape channel requires a
// matching SHAPE_<CHANNEL>_POS/NEG morph target from the source GLB.
// This preserves topology/skin/animation data; bones are NOT retargeted yet.
DeriveResult deriveAvatar(
    const import::GlbMesh& base,
    const AvatarProfile& profile,
    const std::vector<MaterialStyle>& materials = {},
    const std::vector<AttachmentSpec>& attachments = {});

struct RigRemapResult {
    bool ok = false;
    std::string error;
    std::vector<int> attachmentToBaseJoint;
};
// Checks bone names and parent hierarchy. Does not perform GPU skinning
// or garment bind-pose alignment; those are later native runtime stages.
RigRemapResult buildRigRemap(const import::GlbMesh& base,
                             const import::GlbMesh& garment);

} // namespace hakui::avatar
