#include "avatar/HakuiSkeleton.hpp"

#include <SDL3/SDL_log.h>

#if HAKUI_HAS_IMVU_CAL3D
#include <cal3d/BoostToStd.h>
#include <cal3d/corebone.h>
#include <cal3d/coreskeleton.h>
#include <cal3d/skeleton.h>

class HakuiSkeleton::Impl {
public:
    CalCoreSkeletonPtr core;
    std::unique_ptr<CalSkeleton> runtime;
    std::vector<AvatarAttachment> attachments;

    int addBone(const std::string& name, int parent)
    {
        CalCoreBonePtr bone(new CalCoreBone(name, parent));
        return static_cast<int>(core->addCoreBone(bone));
    }

    void rebuildRuntime()
    {
        runtime.reset(new CalSkeleton(core));
        runtime->resetPose();
        runtime->calculateAbsolutePose();
    }
};
#else
class HakuiSkeleton::Impl {
public:
    std::size_t fallbackBoneCount = 0;
    std::vector<AvatarAttachment> attachments;
};
#endif

HakuiSkeleton::HakuiSkeleton() : impl_(std::make_unique<Impl>()) {}
HakuiSkeleton::~HakuiSkeleton() = default;
HakuiSkeleton::HakuiSkeleton(HakuiSkeleton&&) noexcept = default;
HakuiSkeleton& HakuiSkeleton::operator=(HakuiSkeleton&&) noexcept = default;

bool HakuiSkeleton::buildDefaultHumanoid()
{
    impl_->attachments.clear();

#if HAKUI_HAS_IMVU_CAL3D
    impl_->core = CalCoreSkeletonPtr(new CalCoreSkeleton());

    const int root      = impl_->addBone("Root", -1);
    const int pelvis    = impl_->addBone("Pelvis", root);
    const int spine01   = impl_->addBone("Spine.01", pelvis);
    const int spine02   = impl_->addBone("Spine.02", spine01);
    const int chest     = impl_->addBone("Chest", spine02);
    const int neck      = impl_->addBone("Neck", chest);
    const int head      = impl_->addBone("Head", neck);

    const int clavicleL = impl_->addBone("Clavicle.L", chest);
    const int upperArmL = impl_->addBone("UpperArm.L", clavicleL);
    const int lowerArmL = impl_->addBone("LowerArm.L", upperArmL);
    const int handL     = impl_->addBone("Hand.L", lowerArmL);

    const int clavicleR = impl_->addBone("Clavicle.R", chest);
    const int upperArmR = impl_->addBone("UpperArm.R", clavicleR);
    const int lowerArmR = impl_->addBone("LowerArm.R", upperArmR);
    const int handR     = impl_->addBone("Hand.R", lowerArmR);

    const int thighL = impl_->addBone("Thigh.L", pelvis);
    const int shinL  = impl_->addBone("Shin.L", thighL);
    const int footL  = impl_->addBone("Foot.L", shinL);
    impl_->addBone("Toe.L", footL);

    const int thighR = impl_->addBone("Thigh.R", pelvis);
    const int shinR  = impl_->addBone("Shin.R", thighR);
    const int footR  = impl_->addBone("Foot.R", shinR);
    impl_->addBone("Toe.R", footR);

    (void)head;
    (void)handL;
    (void)handR;

    impl_->attachments = {
        {"body", "Pelvis", AttachmentCategory::Body},
        {"head", "Head", AttachmentCategory::Head},
        {"hair", "Head", AttachmentCategory::Hair},
        {"face", "Head", AttachmentCategory::Face},
        {"neck", "Neck", AttachmentCategory::Neck},
        {"torso", "Chest", AttachmentCategory::Torso},
        {"back", "Chest", AttachmentCategory::Back},
        {"waist", "Pelvis", AttachmentCategory::Waist},
        {"hand.left", "Hand.L", AttachmentCategory::LeftHand},
        {"hand.right", "Hand.R", AttachmentCategory::RightHand},
        {"foot.left", "Foot.L", AttachmentCategory::LeftFoot},
        {"foot.right", "Foot.R", AttachmentCategory::RightFoot},
        {"skateboard", "Hand.R", AttachmentCategory::Skateboard},
        {"bmx", "Pelvis", AttachmentCategory::BMX},
        {"vehicle.seat", "Pelvis", AttachmentCategory::VehicleSeat}
    };

    impl_->rebuildRuntime();
    SDL_Log("[HAKUI] IMVU-Cal3D bridge online // bones=%zu // attachment-slots=%zu",
            impl_->core->getCoreBones().size(), impl_->attachments.size());
    return true;
#else
    impl_->fallbackBoneCount = 23;
    impl_->attachments = {
        {"body", "Pelvis", AttachmentCategory::Body},
        {"head", "Head", AttachmentCategory::Head},
        {"hair", "Head", AttachmentCategory::Hair},
        {"face", "Head", AttachmentCategory::Face},
        {"torso", "Chest", AttachmentCategory::Torso},
        {"waist", "Pelvis", AttachmentCategory::Waist}
    };
    SDL_Log("[HAKUI] Cal3D disabled // fallback skeleton metadata active");
    return true;
#endif
}

std::size_t HakuiSkeleton::boneCount() const
{
#if HAKUI_HAS_IMVU_CAL3D
    return impl_->core ? impl_->core->getCoreBones().size() : 0;
#else
    return impl_->fallbackBoneCount;
#endif
}

bool HakuiSkeleton::ready() const
{
#if HAKUI_HAS_IMVU_CAL3D
    return impl_->core != nullptr && impl_->runtime != nullptr;
#else
    return impl_->fallbackBoneCount > 0;
#endif
}

const std::vector<AvatarAttachment>& HakuiSkeleton::attachmentSlots() const
{
    return impl_->attachments;
}
