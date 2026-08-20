#pragma once

#include <optional>
#include <string>

#include "spiral/aum/AUMField.hpp"
#include "spiral/bus/EtherBus.hpp"
#include "spiral/bus/RouterBus.hpp"
#include "spiral/core/StopGate.hpp"
#include "spiral/crystal/CrystalGrid.hpp"
#include "spiral/crystal/CrystalHost.hpp"
#include "spiral/engine/PressureRail.hpp"
#include "spiral/engine/SteamEngine.hpp"
#include "spiral/ledger/MonolithLedger.hpp"
#include "spiral/routing/RouteTable.hpp"
#include "spiral/state/StateStore.hpp"
#include "spiral/wheel/OctopusWheel.hpp"

namespace spiral {

// SpiralKernel is plumbing/orchestration only.
// It owns clocks, transport, state containers, routing, audit, module ownership,
// and lifecycle order. It does not decide policy and it does not choose actions.
class SpiralKernel {
public:
    SpiralKernel();

    void tick(float dtSeconds, TimePoint now = Clock::now());

    RouterBus& router() noexcept;
    RouteTable& routes() noexcept;

    MonolithLedger& monolith() noexcept;
    const MonolithLedger& monolith() const noexcept;

    StateStore& stateStore() noexcept;
    const StateStore& stateStore() const noexcept;

    EtherBus& etherBus() noexcept;
    PressureRail& pressureRail() noexcept;
    SteamEngine& steamEngine() noexcept;
    AUMField& aumField() noexcept;
    CrystalGrid& crystalGrid() noexcept;
    CrystalHost& crystalHost() noexcept;
    const CrystalHost& crystalHost() const noexcept;

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

    // Resolve an address if needed, then put the packet on the Pressure Rail.
    // Route selection is deterministic first-match-wins. A route miss is logged
    // as an error event and returns false; the kernel invents no fallback policy.
    bool dispatch(
        Signal signal,
        float pressureCharge = 0.0f,
        float heatCharge = 0.0f
    );

private:
    Signal makeWheelTransition(const char* topic, std::size_t notch) const;
    void applyTransition(Signal& signal);
    void emitRouteMiss(const Signal& signal);

private:
    RouterBus router_;
    MonolithLedger monolith_;
    StateStore stateStore_;
    RouteTable routes_;

    SteamEngine steamEngine_;
    PressureRail pressureRail_;
    EtherBus etherBus_;

    MindWheel mindWheel_;
    CodingWheel codingWheel_;
    StopGate stopGate_;

    AUMField aumField_;
    CrystalGrid crystalGrid_;

    // Declared after CrystalGrid so it is destroyed first and can detach every
    // owned module before the Grid itself disappears.
    CrystalHost crystalHost_;
};

} // namespace spiral
