#include <cassert>
#include <cstddef>

#include "avatar/HakuiSkeleton.hpp"
#include "spiral/SpiralKernel.hpp"
#include "spiral/crystal/ImvuSkeletonCrystal.hpp"
#include "spiral/crystal/backend/ImvuCal3DBackend.hpp"

namespace {

void hakui_rig_translates_into_optional_imvu_runtime()
{
    HakuiSkeleton rig;
    assert(rig.buildDefaultHumanoid());
    assert(rig.ready());
    assert(rig.boneCount() > 0);

    spiral::ImvuCal3DBackend backend(rig);
    spiral::ImvuSkeletonCrystal crystal(backend.hooks());
    spiral::SpiralKernel kernel;

    // The backend is an AUM crystal, not a Hakui boot dependency.
    assert(kernel.crystalGrid().attach(crystal, 0, 0));
    assert(kernel.crystalGrid().requestEmerge(crystal.id()));

    // Initial A phase performs emergence.
    kernel.tick(0.01f);

    assert(crystal.state() == spiral::Crystal::State::Sustaining);
    assert(crystal.healthy());
    assert(backend.runtimeBoneCount() == rig.boneCount());

    // Runtime signals travel through Router Bus into the crystal.
    spiral::Signal reset;
    reset.kind = spiral::SignalKind::Capability;
    reset.source = "spec";
    reset.destination = std::string(crystal.name());
    reset.topic = "skeleton.reset_pose";
    assert(kernel.dispatch(reset));

    // Return is requested independently, then honored during M phase.
    assert(kernel.crystalGrid().requestReturn(crystal.id()));

    // From ~0.01 sec, +6.1 sec enters the M phase of the 9-second AUM cycle.
    kernel.tick(6.1f);

    assert(crystal.state() == spiral::Crystal::State::Dormant);
    assert(crystal.healthy());
    assert(backend.runtimeBoneCount() == 0);
}

} // namespace

int main()
{
    hakui_rig_translates_into_optional_imvu_runtime();
    return 0;
}
