#pragma once

#include <optional>
#include <string>

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

    const MindWheel& mindWheel() const noexcept;
    const CodingWheel& codingWheel() const noexcept;
    WheelSnapshot wheelSnapshot() const noexcept;

    // Labels are configuration, not behavior. Engine code deliberately does
    // not guess the historical eight notch names.
    bool setMindNotchLabel(std::size_t notch, std::string label);
    bool setCodingNotchLabel(std::size_t notch, std::string label);

    // Wheel posture changes MUST ride Ether Bus. There is intentionally no
    // direct public select() path.
    bool requestMindPolicy(std::size_t notch, TimePoint now = Clock::now());
    bool requestCodingAction(std::size_t notch, TimePoint now = Clock::now());

    // Stop authority is supplied by policy; the kernel only enforces the gate.
    void setMayStopFromPolicy(bool mayStop);
    bool mayStop() const noexcept;

    // Generic transition path for other future mode changes.
    bool beginTransition(const Signal& transitionSignal, TimePoint now = Clock::now());

    // Explicit release. If the passenger is a wheel transition, its notch is
    // activated here and only here, after the enforced transit window.
    std::optional<Signal> disembarkTransition();

private:
    Signal makeWheelTransition(const char* topic, std::size_t notch) const;
    void applyTransition(const Signal& signal);

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
