#pragma once

#include "avatar/HakuiSkeleton.hpp"
#include "spiral/crystal/CrystalModule.hpp"
#include "spiral/crystal/ImvuSkeletonCrystal.hpp"
#include "spiral/crystal/backend/ImvuCal3DBackend.hpp"

namespace spiral {

// One owned package for the optional IMVU-Cal3D capability.
//
// Member order is deliberate: backend_ is constructed first and destroyed
// last; crystal_ is destroyed first, so hook targets can never outlive the
// backend object they reference.
class ImvuCal3DModule final : public CrystalModule {
public:
    explicit ImvuCal3DModule(const HakuiSkeleton& rig);

    Crystal& crystal() noexcept override;
    const Crystal& crystal() const noexcept override;

    ImvuCal3DBackend& backend() noexcept;
    const ImvuCal3DBackend& backend() const noexcept;

private:
    ImvuCal3DBackend backend_;
    ImvuSkeletonCrystal crystal_;
};

} // namespace spiral
