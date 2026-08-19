#pragma once

#include "spiral/wheel/OctopusWheel.hpp"

namespace spiral {

// Stop authority belongs to policy, not action. Requiring a MindWheel argument
// makes it impossible to authorize stop through CodingWheel by type accident.
class StopGate {
public:
    void setPolicy(const MindWheel& mind, bool mayStop) noexcept
    {
        (void)mind;
        mayStop_ = mayStop;
    }

    bool mayStop() const noexcept
    {
        return mayStop_;
    }

private:
    bool mayStop_ = false;
};

} // namespace spiral
