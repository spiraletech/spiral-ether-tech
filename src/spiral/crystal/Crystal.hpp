#pragma once

#include <string_view>

#include "spiral/core/SpiralTypes.hpp"

namespace spiral {

class RouterBus;

// A crystal is a capability node, not an engine owner.
// It may emerge, sustain, and return independently of Hakui core boot.
class Crystal {
public:
    enum class State {
        Dormant,
        Emerging,
        Sustaining,
        Returning,
        Faulted
    };

    virtual ~Crystal() = default;

    virtual CrystalId id() const noexcept = 0;
    virtual std::string_view name() const noexcept = 0;

    virtual bool emerge(RouterBus& bus) = 0;
    virtual void sustain(float dtSeconds) = 0;
    virtual void returnToDormant() = 0;

    virtual void onSignal(const Signal& signal) = 0;

    virtual State state() const noexcept = 0;
    virtual bool healthy() const noexcept = 0;
};

} // namespace spiral
