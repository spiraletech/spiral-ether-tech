#pragma once

#include "spiral/bus/RouterBus.hpp"
#include "spiral/engine/SteamEngine.hpp"

namespace spiral {

// Pressure Rail carries steam + signal together while keeping their semantics
// separate: SteamEngine owns energy/heat; RouterBus owns delivery.
class PressureRail {
public:
    PressureRail(RouterBus& bus, SteamEngine& engine)
        : bus_(bus), engine_(engine) {}

    void send(Signal signal, float pressureCharge = 0.0f, float heatCharge = 0.0f)
    {
        engine_.charge(pressureCharge, heatCharge);
        signal.pressure = engine_.telemetry().pressure;
        signal.timestamp = Clock::now();
        bus_.emit(signal);
    }

    void faultDrain(Signal errorSignal)
    {
        engine_.drain();
        errorSignal.kind = SignalKind::Error;
        errorSignal.pressure = engine_.telemetry().pressure;
        errorSignal.timestamp = Clock::now();
        bus_.emit(errorSignal);
    }

private:
    RouterBus& bus_;
    SteamEngine& engine_;
};

} // namespace spiral
