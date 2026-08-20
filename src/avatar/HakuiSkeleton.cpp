#include "avatar/HakuiSkeleton.hpp"

#include <utility>

bool HakuiSkeleton::buildDefaultHumanoid()
{
    bones_.clear();
    attachments_.clear();

    const int root      = addBone("Root", -1);
    const int pelvis    = addBone("Pelvis", root);
    const int spine01   = addBone("Spine.01", pelvis);
    const int spine02   = addBone("Spine.02", spine01);
    const int chest     = addBone("Chest", spine02);
    const int neck      = addBone("Neck", chest);
    const int head      = addBone("Head", neck);

    const int clavicleL = addBone("Clavicle.L", chest);
    const int upperArmL = addBone("UpperArm.L", clavicleL);
    const int lowerArmL = addBone("LowerArm.L", upperArmL);
    const int handL     = addBone("Hand.L", lowerArmL);

    const int clavicleR = addBone("Clavicle.R", chest);
    const int upperArmR = addBone("UpperArm.R", clavicleR);
    const int lowerArmR = addBone("LowerArm.R", upperArmR);
    const int handR     = addBone("Hand.R", lowerArmR);

    const int thighL = addBone("Thigh.L", pelvis);
    const int shinL  = addBone("Shin.L", thighL);
    const int footL  = addBone("Foot.L", shinL);
    addBone("Toe.L", footL);

    const int thighR = addBone("Thigh.R", pelvis);
    const int shinR  = addBone("Shin.R", thighR);
    const int footR  = addBone("Foot.R", shinR);
    addBone("Toe.R", footR);

    (void)head;
    (void)handL;
    (void)handR;

    attachments_ = {
        {"body",         "Pelvis", AttachmentCategory::Body},
        {"head",         "Head",   AttachmentCategory::Head},
        {"hair",         "Head",   AttachmentCategory::Hair},
        {"face",         "Head",   AttachmentCategory::Face},
        {"neck",         "Neck",   AttachmentCategory::Neck},
        {"torso",        "Chest",  AttachmentCategory::Torso},
        {"back",         "Chest",  AttachmentCategory::Back},
        {"waist",        "Pelvis", AttachmentCategory::Waist},
        {"hand.left",    "Hand.L", AttachmentCategory::LeftHand},
        {"hand.right",   "Hand.R", AttachmentCategory::RightHand},
        {"foot.left",    "Foot.L", AttachmentCategory::LeftFoot},
        {"foot.right",   "Foot.R", AttachmentCategory::RightFoot},
        {"skateboard",   "Hand.R", AttachmentCategory::Skateboard},
        {"bmx",          "Pelvis", AttachmentCategory::BMX},
        {"vehicle.seat", "Pelvis", AttachmentCategory::VehicleSeat}
    };

    return true;
}

std::size_t HakuiSkeleton::boneCount() const noexcept
{
    return bones_.size();
}

bool HakuiSkeleton::ready() const noexcept
{
    return !bones_.empty();
}

const std::vector<HakuiBoneDefinition>& HakuiSkeleton::bones() const noexcept
{
    return bones_;
}

const std::vector<AvatarAttachment>& HakuiSkeleton::attachmentSlots() const noexcept
{
    return attachments_;
}

int HakuiSkeleton::findBone(std::string_view name) const noexcept
{
    for (std::size_t i = 0; i < bones_.size(); ++i) {
        if (bones_[i].name == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int HakuiSkeleton::addBone(std::string name, int parent)
{
    const int index = static_cast<int>(bones_.size());
    bones_.push_back(HakuiBoneDefinition{std::move(name), parent});
    return index;
}
