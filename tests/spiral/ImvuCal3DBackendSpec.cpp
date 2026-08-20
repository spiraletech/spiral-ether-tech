#include <cassert>
#include <cstddef>
#include <memory>
#include <string>

#include "avatar/HakuiSkeleton.hpp"
#include "spiral/SpiralKernel.hpp"
#include "spiral/crystal/backend/ImvuCal3DModule.hpp"

namespace {

void hakui_rig_mounts_as_optional_imvu_crystal_module()
{
    HakuiSkeleton rig;
    assert(rig.buildDefaultHumanoid());
    assert(rig.ready());
    assert(rig.boneCount() > 0);

    spiral::SpiralKernel kernel;

    auto module = std::make_unique<spiral::ImvuCal3DModule>(rig);
    auto* imvuModule = module.get();
    const spiral::CrystalId crystalId = imvuModule->crystal().id();

    // CrystalHost owns both the crystal and the backend lifetime as one module.
    assert(kernel.crystalHost().mount(std::move(module), 0, 0));
    assert(kernel.crystalHost().size() == 1);
    assert(kernel.crystalHost().find(crystalId) != nullptr);
    assert(kernel.crystalHost().requestEmerge(crystalId));

    // Initial A phase performs emergence.
    kernel.tick(0.01f);

    assert(imvuModule->crystal().state() == spiral::Crystal::State::Sustaining);
    assert(imvuModule->crystal().healthy());
    assert(imvuModule->backend().runtimeBoneCount() == rig.boneCount());

    // Runtime signals travel through Router Bus into the mounted crystal.
    spiral::Signal reset;
    reset.kind = spiral::SignalKind::Capability;
    reset.source = "spec";
    reset.destination = std::string(imvuModule->crystal().name());
    reset.topic = "skeleton.reset_pose";
    assert(kernel.dispatch(reset));

    // Return is requested independently, then honored during M phase.
    assert(kernel.crystalHost().requestReturn(crystalId));

    // From ~0.01 sec, +6.1 sec enters the M phase of the 9-second AUM cycle.
    kernel.tick(6.1f);

    assert(imvuModule->crystal().state() == spiral::Crystal::State::Dormant);
    assert(imvuModule->crystal().healthy());
    assert(imvuModule->backend().runtimeBoneCount() == 0);

    // Unmount detaches Grid references before module/backend memory is deleted.
    assert(kernel.crystalHost().unmount(crystalId));
    assert(kernel.crystalHost().size() == 0);
    assert(kernel.crystalGrid().find(crystalId) == nullptr);
}

} // namespace

int main()
{
    hakui_rig_mounts_as_optional_imvu_crystal_module();
    return 0;
}
