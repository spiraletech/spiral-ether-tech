#pragma once

#include <string_view>

enum class AUMPhase {
    Dormant,
    A_Emergence,
    U_Sustain,
    M_Return,
    Failed
};

class AUMCrystal {
public:
    virtual ~AUMCrystal() = default;

    virtual std::string_view name() const = 0;

    // A — emergence / expansion.
    // Acquire capability, create local state, validate dependencies.
    virtual bool emerge() = 0;

    // U — flow / sustain.
    // Perform the crystal's per-frame work without owning the whole engine.
    virtual void sustain(float dt) = 0;

    // M — compression / return.
    // Release capability and return to a safe dormant state.
    virtual void compress() = 0;

    virtual bool healthy() const = 0;
    virtual AUMPhase phase() const = 0;
};
