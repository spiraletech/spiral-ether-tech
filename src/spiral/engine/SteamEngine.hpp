#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace spiral {

class SteamEngine {
public:
    static constexpr std::size_t kPistonCount = 16;

    enum class State {
        Idle,
        Pressurizing,
        Firing,
        Redline,
        Cooling,
        Maintenance
    };

    struct Telemetry {
        float pressure = 0.0f;
        float temperature = 0.0f;
        float heat = 0.0f;
        float output = 0.0f;
        bool safetyValveOpen = false;
        bool maintenanceDue = false;
        State state = State::Idle;
    };

    SteamEngine();

    // Add steam/pressure to the rail. This is energy only, never policy.
    void charge(float pressure, float heat = 0.0f);

    // Fire the next piston in deterministic order and convert pressure to
    // useful output. The caller asks for demand; the engine owns firing order.
    float fireNext(float demand);

    // Condenser / idle recovery. Non-blocking; called from the core tick.
    void tick(float dtSeconds);

    // Steam drain: explicit fault-isolation path. It vents stored pressure and
    // forces the engine into cooling without deciding what the application does.
    void drain();

    // Maintenance is explicit. Redline can request it; only the caller clears it.
    void completeMaintenance();

    const Telemetry& telemetry() const noexcept;
    std::size_t nextPiston() const noexcept;

private:
    void applySafetyValve();
    void updateState();

private:
    Telemetry telemetry_{};
    std::array<std::size_t, kPistonCount> firingOrder_{};
    std::size_t firingCursor_ = 0;

    float maxSafePressure_ = 100.0f;
    float valveTargetPressure_ = 82.0f;
    float redlineHeat_ = 90.0f;
    float maintenanceHeat_ = 110.0f;
    float condenserRate_ = 10.0f;
    float passivePressureLoss_ = 1.5f;
};

} // namespace spiral
