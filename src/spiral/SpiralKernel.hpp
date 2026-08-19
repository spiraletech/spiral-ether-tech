#pragma once

#include <optional>

#include "spiral/aum/AUMField.hpp"
#include "spiral/bus/EtherBus.hpp"
#include "spiral/bus/RouterBus.hpp"
#include "spiral/core/StopGate.hpp"
#include "spiral/crystal/CrystalGrid.hpp"
#include "spiral/engine/PressureRail.hpp"
#include "spiral/engine/SteamEngine.hpp"
#include "spiral/wheel/OctopusWheel.hpp"

namespace spiral {

// SpiralKernel is plumbing/orchestration only.
// It owns clocks, transport, state containers, and lifecycle order.
// It does not decide policy and it does not choose actions.
class SpiralKernel {
public:
    SpiralKernel();

    void tick(float dtSeconds, TimePoint now = Clock::now());

    RouterBus& router() noexcept;
    EtherBus& etherBus() noexcept;
    PressureRail& pressureRail() noexcept;
    SteamEngine& steamEngine() noexcept;
    AUMField& aumField() noexcept;
    CrystalGrid& crystalGrid() noexcept;

    MindWheel& mindWheel() noexcept;
    CodingWheel& codingWheel() noexcept;

    const MindWheel& mindWheel() const noexcept;
    const CodingWheel& codingWheel() const noexcept;

    WheelSnapshot wheelSnapshot() const noexcept;

    // Mode changes are deliberately explicit and independent.
    bool selectMindPolicy(std::size_t notch);
    bool selectCodingAction(std::size_t notch);

    // Stop authority is supplied by policy; the kernel only enforces the gate.
    void setMayStopFromPolicy(bool mayStop);
    bool mayStop() const noexcept;

    // Ether Bus transition path. There is no bypass/skip method.
    bool beginTransition(const Signal& transitionSignal, TimePoint now = Clock::now());
    std::optional<Signal> disembarkTransition();

private:
    RouterBus router_;
    SteamEngine steamEngine_;
    PressureRail pressureRail_;
    EtherBus etherBus_;

    MindWheel mindWheel_;
    CodingWheel codingWheel_;
    StopGate stopGate_;

    AUMField aumField_;
    CrystalGrid crystalGrid_;
};

} // namespace spiral
