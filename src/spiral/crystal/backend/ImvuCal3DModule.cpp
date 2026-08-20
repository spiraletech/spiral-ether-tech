#include "spiral/crystal/backend/ImvuCal3DModule.hpp"

namespace spiral {

ImvuCal3DModule::ImvuCal3DModule(const HakuiSkeleton& rig)
    : backend_(rig),
      crystal_(backend_.hooks())
{
}

Crystal& ImvuCal3DModule::crystal() noexcept
{
    return crystal_;
}

const Crystal& ImvuCal3DModule::crystal() const noexcept
{
    return crystal_;
}

ImvuCal3DBackend& ImvuCal3DModule::backend() noexcept
{
    return backend_;
}

const ImvuCal3DBackend& ImvuCal3DModule::backend() const noexcept
{
    return backend_;
}

} // namespace spiral
