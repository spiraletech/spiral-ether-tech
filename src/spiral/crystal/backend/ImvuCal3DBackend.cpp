#include "spiral/crystal/backend/ImvuCal3DBackend.hpp"

#include <memory>
#include <utility>

#include <cal3d/BoostToStd.h>
#include <cal3d/corebone.h>
#include <cal3d/coreskeleton.h>
#include <cal3d/skeleton.h>

namespace spiral {

class ImvuCal3DBackend::Impl {
public:
    explicit Impl(const HakuiSkeleton& rigRef)
        : rig(&rigRef) {}

    const HakuiSkeleton* rig = nullptr;
    CalCoreSkeletonPtr core;
    std::unique_ptr<CalSkeleton> runtime;
};

ImvuCal3DBackend::ImvuCal3DBackend(const HakuiSkeleton& rig)
    : impl_(std::make_unique<Impl>(rig))
{
}

ImvuCal3DBackend::~ImvuCal3DBackend() = default;
ImvuCal3DBackend::ImvuCal3DBackend(ImvuCal3DBackend&&) noexcept = default;
ImvuCal3DBackend& ImvuCal3DBackend::operator=(ImvuCal3DBackend&&) noexcept = default;

ImvuSkeletonCrystal::BackendHooks ImvuCal3DBackend::hooks()
{
    ImvuSkeletonCrystal::BackendHooks out;
    out.available = [this]() { return available(); };
    out.boot = [this]() { return boot(); };
    out.tick = [this](float dtSeconds) { tick(dtSeconds); };
    out.shutdown = [this]() { shutdown(); };
    out.onSignal = [this](const Signal& signal) { onSignal(signal); };
    return out;
}

bool ImvuCal3DBackend::available() const noexcept
{
    return impl_ && impl_->rig && impl_->rig->ready();
}

bool ImvuCal3DBackend::boot()
{
    if (!available()) {
        return false;
    }

    impl_->runtime.reset();
    impl_->core = CalCoreSkeletonPtr(new CalCoreSkeleton());

    for (const HakuiBoneDefinition& bone : impl_->rig->bones()) {
        CalCoreBonePtr calBone(new CalCoreBone(bone.name, bone.parent));
        impl_->core->addCoreBone(calBone);
    }

    impl_->runtime = std::make_unique<CalSkeleton>(impl_->core);
    impl_->runtime->resetPose();
    impl_->runtime->calculateAbsolutePose();

    return impl_->runtime != nullptr;
}

void ImvuCal3DBackend::tick(float dtSeconds)
{
    (void)dtSeconds;

    if (!impl_ || !impl_->runtime) {
        return;
    }

    // Animation tracks are intentionally not owned here yet. For now the
    // backend maintains a valid absolute pose from the Hakui rig definition.
    impl_->runtime->calculateAbsolutePose();
}

void ImvuCal3DBackend::shutdown()
{
    if (!impl_) {
        return;
    }

    impl_->runtime.reset();
    impl_->core.reset();
}

void ImvuCal3DBackend::onSignal(const Signal& signal)
{
    if (!impl_ || !impl_->runtime) {
        return;
    }

    if (signal.topic == "skeleton.reset_pose") {
        impl_->runtime->resetPose();
        impl_->runtime->calculateAbsolutePose();
    }
}

std::size_t ImvuCal3DBackend::runtimeBoneCount() const noexcept
{
    if (!impl_ || !impl_->core) {
        return 0;
    }
    return impl_->core->getCoreBones().size();
}

} // namespace spiral
