#include "spiral/SpiralKernel.hpp"

#include <utility>

namespace spiral {

SpiralKernel::SpiralKernel()
    : monolith_(router_),
      pressureRail_(router_, steamEngine_),
      crystalGrid_(router_, aumField_)
{
}

void SpiralKernel::tick(float dtSeconds, TimePoint now)
{
    // Order is deliberate:
    // 1. thermodynamic state evolves
    // 2. Ether transition clock advances
    // 3. AUM governor advances
    // 4. crystal lifecycle reacts to current AUM phase
    steamEngine_.tick(dtSeconds);
    etherBus_.tick(now);
    aumField_.tick(dtSeconds);
    crystalGrid_.tick(dtSeconds, aumField_.phase());
}

RouterBus& SpiralKernel::router() noexcept
{
    return router_;
}

RouteTable& SpiralKernel::routes() noexcept
{
    return routes_;
}

MonolithLedger& SpiralKernel::monolith() noexcept
{
    return monolith_;
}

const MonolithLedger& SpiralKernel::monolith() const noexcept
{
    return monolith_;
}

EtherBus& SpiralKernel::etherBus() noexcept
{
    return etherBus_;
}

PressureRail& SpiralKernel::pressureRail() noexcept
{
    return pressureRail_;
}

SteamEngine& SpiralKernel::steamEngine() noexcept
{
    return steamEngine_;
}

AUMField& SpiralKernel::aumField() noexcept
{
    return aumField_;
}

CrystalGrid& SpiralKernel::crystalGrid() noexcept
{
    return crystalGrid_;
}

const MindWheel& SpiralKernel::mindWheel() const noexcept
{
    return mindWheel_;
}

const CodingWheel& SpiralKernel::codingWheel() const noexcept
{
    return codingWheel_;
}

WheelSnapshot SpiralKernel::wheelSnapshot() const noexcept
{
    return WheelSnapshot{
        mindWheel_.activeIndex(),
        codingWheel_.activeIndex()
    };
}

bool SpiralKernel::setMindNotchLabel(std::size_t notch, std::string label)
{
    return mindWheel_.setLabel(notch, std::move(label));
}

bool SpiralKernel::setCodingNotchLabel(std::size_t notch, std::string label)
{
    return codingWheel_.setLabel(notch, std::move(label));
}

bool SpiralKernel::requestMindPolicy(std::size_t notch, TimePoint now)
{
    if (notch >= kOctopusNotchCount) {
        return false;
    }
    return beginTransition(makeWheelTransition("wheel.mind", notch), now);
}

bool SpiralKernel::requestCodingAction(std::size_t notch, TimePoint now)
{
    if (notch >= kOctopusNotchCount) {
        return false;
    }
    return beginTransition(makeWheelTransition("wheel.coding", notch), now);
}

void SpiralKernel::setMayStopFromPolicy(bool mayStop)
{
    stopGate_.setPolicy(mindWheel_, mayStop);
}

bool SpiralKernel::mayStop() const noexcept
{
    return stopGate_.mayStop();
}

bool SpiralKernel::beginTransition(const Signal& transitionSignal, TimePoint now)
{
    return etherBus_.board(transitionSignal, now);
}

std::optional<Signal> SpiralKernel::disembarkTransition()
{
    auto signal = etherBus_.disembark();
    if (!signal) {
        return std::nullopt;
    }

    applyTransition(*signal);

    // Only after explicit Disembark does the transition re-enter the live rail.
    dispatch(*signal);
    return signal;
}

bool SpiralKernel::dispatch(Signal signal, float pressureCharge, float heatCharge)
{
    if (signal.destination.empty()) {
        const auto match = routes_.resolve(signal);
        if (!match) {
            emitRouteMiss(signal);
            return false;
        }
        signal.destination = match->destination;
    }

    pressureRail_.send(std::move(signal), pressureCharge, heatCharge);
    return true;
}

Signal SpiralKernel::makeWheelTransition(const char* topic, std::size_t notch) const
{
    Signal signal;
    signal.kind = SignalKind::State;
    signal.source = "spiral.interface";
    signal.destination = "spiral.core";
    signal.topic = topic;
    signal.notch = notch;
    signal.timestamp = Clock::now();
    return signal;
}

void SpiralKernel::applyTransition(const Signal& signal)
{
    if (!signal.notch || *signal.notch >= kOctopusNotchCount) {
        return;
    }

    if (signal.topic == "wheel.mind") {
        mindWheel_.select(*signal.notch);
        return;
    }

    if (signal.topic == "wheel.coding") {
        codingWheel_.select(*signal.notch);
    }
}

void SpiralKernel::emitRouteMiss(const Signal& signal)
{
    Signal error;
    error.kind = SignalKind::Error;
    error.source = "spiral.router";
    error.destination = "spiral.core";
    error.topic = "route.miss";
    error.payload = signal.topic;
    error.timestamp = Clock::now();
    router_.emit(std::move(error));
}

} // namespace spiral
