#include "spiral/engine/SteamEngine.hpp"

#include <algorithm>

namespace spiral {

SteamEngine::SteamEngine()
{
    for (std::size_t i = 0; i < kPistonCount; ++i) {
        firingOrder_[i] = i;
    }
}

void SteamEngine::charge(float pressure, float heat)
{
    if (pressure > 0.0f) {
        telemetry_.pressure += pressure;
    }
    if (heat > 0.0f) {
        telemetry_.heat += heat;
        telemetry_.temperature += heat * 0.5f;
    }

    applySafetyValve();
    updateState();
}

float SteamEngine::fireNext(float demand)
{
    telemetry_.output = 0.0f;

    if (demand <= 0.0f || telemetry_.maintenanceDue) {
        updateState();
        return 0.0f;
    }

    if (telemetry_.pressure <= 0.0f) {
        updateState();
        return 0.0f;
    }

    const std::size_t piston = firingOrder_[firingCursor_];
    (void)piston;
    firingCursor_ = (firingCursor_ + 1) % kPistonCount;

    const float available = std::min(telemetry_.pressure, demand);
    const float efficiency = telemetry_.heat >= redlineHeat_ ? 0.65f : 0.88f;
    const float output = available * efficiency;

    telemetry_.pressure -= available;
    telemetry_.heat += available * 0.16f;
    telemetry_.temperature += available * 0.08f;
    telemetry_.output = output;

    if (telemetry_.heat >= maintenanceHeat_) {
        telemetry_.maintenanceDue = true;
    }

    applySafetyValve();
    updateState();
    return output;
}

void SteamEngine::tick(float dtSeconds)
{
    if (dtSeconds <= 0.0f) {
        return;
    }

    // Condenser behavior: shed heat continuously and let stored pressure leak
    // down slowly while no piston is actively producing output.
    telemetry_.heat = std::max(0.0f, telemetry_.heat - condenserRate_ * dtSeconds);
    telemetry_.temperature = std::max(0.0f, telemetry_.temperature - condenserRate_ * 0.5f * dtSeconds);
    telemetry_.pressure = std::max(0.0f, telemetry_.pressure - passivePressureLoss_ * dtSeconds);
    telemetry_.output = 0.0f;

    applySafetyValve();
    updateState();
}

void SteamEngine::drain()
{
    telemetry_.pressure = 0.0f;
    telemetry_.output = 0.0f;
    telemetry_.safetyValveOpen = true;
    telemetry_.state = State::Cooling;
}

void SteamEngine::completeMaintenance()
{
    telemetry_.maintenanceDue = false;
    telemetry_.heat = std::min(telemetry_.heat, redlineHeat_ * 0.5f);
    telemetry_.temperature = std::min(telemetry_.temperature, redlineHeat_ * 0.35f);
    updateState();
}

const SteamEngine::Telemetry& SteamEngine::telemetry() const noexcept
{
    return telemetry_;
}

std::size_t SteamEngine::nextPiston() const noexcept
{
    return firingOrder_[firingCursor_];
}

void SteamEngine::applySafetyValve()
{
    if (telemetry_.pressure > maxSafePressure_) {
        telemetry_.pressure = valveTargetPressure_;
        telemetry_.safetyValveOpen = true;
    } else if (telemetry_.pressure < valveTargetPressure_) {
        telemetry_.safetyValveOpen = false;
    }
}

void SteamEngine::updateState()
{
    if (telemetry_.maintenanceDue) {
        telemetry_.state = State::Maintenance;
        return;
    }

    if (telemetry_.heat >= redlineHeat_) {
        telemetry_.state = State::Redline;
        return;
    }

    if (telemetry_.output > 0.0f) {
        telemetry_.state = State::Firing;
        return;
    }

    // Stored steam means the engine is charged/pressurized even if the
    // condenser is simultaneously shedding heat.
    if (telemetry_.pressure > 0.0f) {
        telemetry_.state = State::Pressurizing;
        return;
    }

    telemetry_.state = telemetry_.heat > 0.0f ? State::Cooling : State::Idle;
}

} // namespace spiral
