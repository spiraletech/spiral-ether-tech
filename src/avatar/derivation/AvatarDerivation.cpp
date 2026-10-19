#include "avatar/derivation/AvatarDerivation.hpp"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <set>
#include <string>
#include <string_view>
#include <utility>

namespace hakui::avatar {
namespace {
constexpr std::array<std::string_view, kShapeChannelCount> kNames{
    "STATURE", "SHOULDER_WIDTH", "CHEST_VOLUME", "WAIST_WIDTH",
    "HIP_WIDTH", "ARM_LENGTH", "LEG_LENGTH", "HEAD_SIZE",
    "JAW_WIDTH", "NOSE_SHAPE", "EYE_SHAPE", "MOUTH_SHAPE"
};
std::string uppercase(std::string_view s) {
    std::string result;
    result.reserve(s.size());
    for (char c : s) {
        if (c >= 'a' && c <= 'z') result.push_back(static_cast<char>(c - 'a' + 'A'));
        else result.push_back(c);
    }
    return result;
}
std::string normalizedBone(std::string_view text) {
    std::string out;
    out.reserve(text.size());
    for (char c : text) {
        if (c >= 'a' && c <= 'z') out.push_back(static_cast<char>(c - 'a' + 'A'));
        else if ((c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9')) out.push_back(c);
    }
    return out;
}
int findBone(const import::GlbMesh& mesh, std::string_view name) {
    const auto normalized = normalizedBone(name);
    for (std::size_t i = 0; i < mesh.joints.size(); ++i)
        if (normalizedBone(mesh.joints[i].name) == normalized) return static_cast<int>(i);
    return -1;
}
std::string_view slotDefaultBone(std::string_view slot) noexcept {
    if (slot == "HAIR" || slot == "HEADWEAR") return "Head";
    if (slot == "SHIRT_OUTER" || slot == "SHIRT_UNDER" || slot == "BOARD_BACK") return "Chest";
    if (slot == "PANTS" || slot == "FOOTWEAR") return "Pelvis";
    if (slot == "ACCESSORY_NECK") return "Neck";
    if (slot == "HAND_LEFT") return "Hand_L";
    if (slot == "HAND_RIGHT") return "Hand_R";
    return {};
}
bool finite3(const std::array<float, 3>& v) {
    return std::isfinite(v[0]) && std::isfinite(v[1]) && std::isfinite(v[2]);
}
} // namespace

std::string_view shapeChannelName(ShapeChannel c) noexcept {
    const std::size_t i = static_cast<std::size_t>(c);
    return i < kNames.size() ? kNames[i] : std::string_view{};
}

DeriveResult deriveAvatar(const import::GlbMesh& base,
                          const AvatarProfile& profile,
                          const std::vector<MaterialStyle>& materials,
                          const std::vector<AttachmentSpec>& attachments) {
    DeriveResult out{};
    if (base.vertices.empty() || base.indices.empty() || base.joints.empty()) {
        out.error = "Base mannequin requires indexed skinned mesh and nonempty skeleton";
        return out;
    }
    if (base.indices.size() % 3) {
        out.error = "Base mannequin has non-triangle index data";
        return out;
    }
    for (const auto index : base.indices) {
        if (index >= base.vertices.size()) {
            out.error = "Base mannequin index out of bounds";
            return out;
        }
    }
    if (!std::isfinite(profile.uniformScale) || profile.uniformScale < 0.5f || profile.uniformScale > 2.f) {
        out.error = "Avatar uniformScale must be finite and in [0.5, 2.0]";
        return out;
    }
    for (const auto& v : base.vertices) {
        if (!finite3(v.position)) {
            out.error = "Base mannequin contains non-finite vertex coordinates";
            return out;
        }
        float sum = 0.f;
        for (std::size_t k = 0; k < 4; ++k) {
            if (v.joints[k] >= base.joints.size()) {
                out.error = "Base mannequin vertex references nonexistent joint";
                return out;
            }
            if (!std::isfinite(v.weights[k]) || v.weights[k] < 0.f) {
                out.error = "Base mannequin skin weights invalid";
                return out;
            }
            sum += v.weights[k];
        }
        if (!std::isfinite(sum) || std::abs(sum - 1.f) > 0.02f) {
            out.error = "Base mannequin skin weights must sum to one";
            return out;
        }
    }
    std::set<std::string> boneNames;
    for (std::size_t i = 0; i < base.joints.size(); ++i) {
        const auto& joint = base.joints[i];
        const std::string key = normalizedBone(joint.name);
        if (key.empty() || !boneNames.insert(key).second ||
            joint.parent < -1 || joint.parent >= static_cast<int>(i)) {
            out.error = "Base mannequin skeleton must have unique bone names and parent-before-child order";
            return out;
        }
    }
    std::set<std::string> usedSlots;
    std::vector<ResolvedAttachment> resolved;
    for (const auto& spec : attachments) {
        const std::string slot = uppercase(spec.slot);
        const auto defaultBone = slotDefaultBone(slot);
        if (defaultBone.empty()) {
            out.error = "Unknown avatar attachment slot: " + spec.slot;
            return out;
        }
        if (spec.assetPath.empty() || !usedSlots.insert(slot).second) {
            out.error = "Attachment path must be set and each slot unique: " + slot;
            return out;
        }
        const std::string desiredBone = spec.boneName.empty() ? std::string(defaultBone) : spec.boneName;
        const int anchor = findBone(base, desiredBone);
        if (anchor < 0) {
            out.error = "Attachment " + slot + " requires bone: " + desiredBone;
            return out;
        }
        resolved.push_back({spec, base.joints[static_cast<std::size_t>(anchor)].name, anchor});
    }
    std::set<std::string> matNames;
    for (const auto& mat : materials) {
        if (mat.materialName.empty() || !matNames.insert(mat.materialName).second) {
            out.error = "Duplicate/empty material override name";
            return out;
        }
        if (std::any_of(mat.baseColor.begin(), mat.baseColor.end(),
                        [](float x) { return !std::isfinite(x) || x < 0.f || x > 1.f; })) {
            out.error = "Material override values must be finite in [0, 1]";
            return out;
        }
        bool found = false;
        for (const auto& s : base.submeshes) found = found || s.material == mat.materialName;
        if (!found) {
            out.error = "Material override not present in mannequin: " + mat.materialName;
            return out;
        }
    }
    out.avatar.mesh = base;
    out.avatar.profile = profile;
    out.avatar.materials = materials;
    out.avatar.attachments = std::move(resolved);
    for (std::size_t i = 0; i < kShapeChannelCount; ++i) {
        const float strength = profile.shape[i];
        if (!std::isfinite(strength) || strength < -1.f || strength > 1.f) {
            out.error = "Shape channel " + std::string(kNames[i]) + " must be in [-1, 1]";
            out.avatar = {};
            return out;
        }
        if (std::abs(strength) <= 1.e-6f) continue;
        const std::string morphName =
            "SHAPE_" + std::string(kNames[i]) + (strength > 0.f ? "_POS" : "_NEG");
        const import::MorphTarget* morph = nullptr;
        for (const auto& target : base.morphTargets)
            if (uppercase(target.name) == morphName) { morph = &target; break; }
        if (!morph) {
            out.error = "Required authored morph target missing: " + morphName;
            out.avatar = {};
            return out;
        }
        if (morph->positionDeltas.size() != base.vertices.size()) {
            out.error = "Morph target vertex count mismatch: " + morphName;
            out.avatar = {};
            return out;
        }
        for (std::size_t v = 0; v < base.vertices.size(); ++v) {
            if (!finite3(morph->positionDeltas[v])) {
                out.error = "Non-finite morph target data: " + morphName;
                out.avatar = {};
                return out;
            }
            auto& position = out.avatar.mesh.vertices[v].position;
            const float alpha = std::abs(strength);
            for (std::size_t k = 0; k < 3; ++k) position[k] += alpha * morph->positionDeltas[v][k];
        }
        out.avatar.consumedShapeKeys.push_back(morph->name);
    }
    out.ok = true;
    return out;
}

RigRemapResult buildRigRemap(const import::GlbMesh& base,
                             const import::GlbMesh& garment) {
    RigRemapResult result{};
    if (base.joints.empty() || garment.joints.empty()) {
        result.error = "Both base mannequin and garment require a skeleton";
        return result;
    }
    result.attachmentToBaseJoint.reserve(garment.joints.size());
    for (const auto& bone : garment.joints) {
        const int index = findBone(base, bone.name);
        if (index < 0) {
            result.error = "Garment references noncanonical mannequin bone: " + bone.name;
            result.attachmentToBaseJoint.clear();
            return result;
        }
        result.attachmentToBaseJoint.push_back(index);
    }
    for (std::size_t i = 0; i < garment.joints.size(); ++i) {
        const int parent = garment.joints[i].parent;
        if (parent < -1 || parent >= static_cast<int>(garment.joints.size()) ||
            parent == static_cast<int>(i)) {
            result.error = "Garment skeleton parent index invalid";
            result.attachmentToBaseJoint.clear();
            return result;
        }
        if (parent >= 0) {
            const int baseIndex = result.attachmentToBaseJoint[i];
            const int baseParent = result.attachmentToBaseJoint[static_cast<std::size_t>(parent)];
            if (base.joints[static_cast<std::size_t>(baseIndex)].parent != baseParent) {
                result.error = "Garment skeleton hierarchy does not match base: " + garment.joints[i].name;
                result.attachmentToBaseJoint.clear();
                return result;
            }
        }
    }
    result.ok = true;
    return result;
}

} // namespace hakui::avatar
