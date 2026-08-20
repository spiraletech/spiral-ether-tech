#pragma once

#include "spiral/crystal/Crystal.hpp"

namespace spiral {

// Ownership boundary for a capability crystal and any resources required to
// keep that crystal alive (runtime backend, caches, adapters, handles, etc.).
//
// CrystalGrid owns lifecycle/routing relationships but never owns module
// memory. CrystalHost owns modules. This separation prevents dangling backend
// hooks while preserving the Grid as pure orchestration plumbing.
class CrystalModule {
public:
    virtual ~CrystalModule() = default;

    virtual Crystal& crystal() noexcept = 0;
    virtual const Crystal& crystal() const noexcept = 0;
};

} // namespace spiral
