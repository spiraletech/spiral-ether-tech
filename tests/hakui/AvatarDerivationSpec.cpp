#include "avatar/derivation/AvatarDerivation.hpp"

#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <string>
#include <utility>

using namespace hakui;

namespace {
void check(bool cond, const char* reason) {
    if (!cond) { std::cerr << "[FAIL] " << reason << '\n'; std::exit(1); }
}
import::Joint joint(std::string name, int parent) {
    import::Joint j{};
    j.name = std::move(name);
    j.parent = parent;
    j.inverseBind = {1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1};
    return j;
}
import::GlbMesh base() {
    import::GlbMesh m{};
    m.joints = {joint("Pelvis",-1), joint("Waist",0), joint("Chest",1),
                joint("Neck",2), joint("Head",3), joint("Hand_L",2), joint("Hand_R",2)};
    m.vertices.resize(4);
    m.vertices[0].position = {0.00f, 1.0f, 0.f};
    m.vertices[1].position = {0.3f, 1.0f, 0.f};
    m.vertices[2].position = {0.0f, 1.5f, 0.f};
    m.vertices[3].position = {0.3f, 1.5f, 0.f};
    for (auto& v : m.vertices) {
        v.joints = {2,0,0,0};
        v.weights = {1.f,0,0,0};
    }
    m.indices = {0,1,2,1,3,2};
    m.submeshes.push_back({"Body","Mat_Skin",0,6});
    m.morphTargets.push_back({"SHAPE_SHOULDER_WIDTH_POS",
        {{0,0,0},{0.15f,0,0},{0,0,0},{0.15f,0,0}}});
    m.morphTargets.push_back({"SHAPE_SHOULDER_WIDTH_NEG",
        {{0,0,0},{-0.12f,0,0},{0,0,0},{-0.12f,0,0}}});
    m.animationChannels.push_back({"WALK",2,"translation","LINEAR",
        {0.f,1.f},{{{0,0,0,0},{0,0.02f,0,0}}}});
    return m;
}
}

int main() {
    const auto mannequin = base();
    avatar::AvatarProfile profile;
    profile.id = "Avatar_A";
    profile.shape[static_cast<std::size_t>(avatar::ShapeChannel::ShoulderWidth)] = 0.5f;
    profile.uniformScale = 1.15f;
    const auto result = avatar::deriveAvatar(mannequin,profile,
        {{"Mat_Skin",{0.6f,0.4f,0.3f,1.f}}},
        {{"HAIR","assets/hair_01.glb","",false},
         {"SHIRT_OUTER","assets/shirt_01.glb","",true}});
    check(result.ok,"authored morph derives avatar successfully");
    check(std::abs(result.avatar.mesh.vertices[1].position[0]-0.375f)<1.e-6f,
          "positive morph deformation");
    check(mannequin.vertices[1].position[0]==0.3f,
          "immutable source mannequin");
    check(result.avatar.mesh.indices==mannequin.indices,
          "triangle topology preserved");
    check(result.avatar.mesh.joints.size()==mannequin.joints.size(),
          "joint hierarchy preserved");
    check(result.avatar.mesh.animationChannels.size()==mannequin.animationChannels.size(),
          "animation data preserved");
    check(result.avatar.profile.uniformScale==1.15f,
          "scale carried as instance metadata");
    check(result.avatar.attachments.size()==2 &&
          result.avatar.attachments[0].anchorJointIndex==4 &&
          result.avatar.attachments[1].anchorJointIndex==2,
          "hair and clothing resolve canonical anchors");
    auto repeat=avatar::deriveAvatar(mannequin,profile);
    check(repeat.ok && repeat.avatar.mesh.vertices[1].position==
          result.avatar.mesh.vertices[1].position,"deterministic derivation");
    profile.shape[static_cast<std::size_t>(avatar::ShapeChannel::ShoulderWidth)]=-0.5f;
    auto neg=avatar::deriveAvatar(mannequin,profile);
    check(neg.ok && std::abs(neg.avatar.mesh.vertices[1].position[0]-0.24f)<1.e-6f,
          "negative morph target is separate");
    profile.shape[static_cast<std::size_t>(avatar::ShapeChannel::Stature)]=0.2f;
    auto missing=avatar::deriveAvatar(mannequin,profile);
    check(!missing.ok && missing.error.find("SHAPE_STATURE_POS")!=std::string::npos,
          "missing target reports error instead of inventing geometry");
    profile.shape[static_cast<std::size_t>(avatar::ShapeChannel::Stature)]=0;
    profile.shape[static_cast<std::size_t>(avatar::ShapeChannel::ShoulderWidth)]=2.f;
    check(!avatar::deriveAvatar(mannequin,profile).ok,
          "out-of-range shape rejected");
    profile.shape[static_cast<std::size_t>(avatar::ShapeChannel::ShoulderWidth)]=0;
    profile.uniformScale=std::numeric_limits<float>::quiet_NaN();
    check(!avatar::deriveAvatar(mannequin,profile).ok,
          "non-finite world scale rejected");
    profile.uniformScale=1.f;
    check(!avatar::deriveAvatar(mannequin,profile, {},
          {{"HAIR","hair.glb","BadBone",false}}).ok,
          "unknown anchor rejected");
    check(!avatar::deriveAvatar(mannequin,profile, {},
          {{"HAIR","hair.glb","",false},{"HAIR","other.glb","",false}}).ok,
          "duplicate attachment slots rejected");
    auto garment=mannequin;
    garment.joints={joint("Chest",1),joint("Waist",-1),joint("Hand.L",0)};
    const auto rig=avatar::buildRigRemap(mannequin,garment);
    check(rig.ok && rig.attachmentToBaseJoint==std::vector<int>({2,1,5}),
          "garment bones map by normalized names");
    garment.joints[2].name="Unknown";
    check(!avatar::buildRigRemap(mannequin,garment).ok,
          "incompatible outfit skeleton rejected");
    auto invalid=mannequin;
    invalid.indices.push_back(20);
    check(!avatar::deriveAvatar(invalid,profile).ok,"invalid triangle index rejected");
    invalid=mannequin;
    invalid.vertices[0].joints[0]=250;
    check(!avatar::deriveAvatar(invalid,profile).ok,"invalid joint index rejected");
    std::cout<<"[PASS] HAKUI M4B avatar derivation/attachments/rig matching\n";
}
