#pragma once

#include <cstddef>
#include <memory>

#include "avatar/HakuiSkeleton.hpp"
#include "spiral/crystal/ImvuSkeletonCrystal.hpp"

namespace spiral {

// Optional translator from Hakui's first-party rig definition into IMVU-Cal3D.
//
// The public header intentionally exposes no Cal3D types. All legacy runtime
// objects live inside the private implementation in the .cpp translation unit.
class ImvuCal3DBackend {
public:
    explicit ImvuCal3DBackend(const HakuiSkeleton& rig);
    ~ImvuCal3DBackend();

    ImvuCal3DBackend(ImvuCal3DBackend&&) noexcept;
    ImvuCal3DBackend& operator=(ImvuCal3DBackend&&) noexcept;

    ImvuCal3DBackend(const ImvuCal3DBackend&) = delete;
    ImvuCal3DBackend& operator=(const ImvuCal3DBackend&) = delete;

    // Returned hooks capture this backend. The backend must outlive the crystal
    // that uses the hook table.
    ImvuSkeletonCrystal::BackendHooks hooks();

    bool available() const noexcept;
    bool boot();
    void tick(float dtSeconds);
    void shutdown();
    void onSignal(const Signal& signal);

    std::size_t runtimeBoneCount() const noexcept;

private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace spiral
