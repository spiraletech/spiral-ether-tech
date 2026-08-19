#include "spiral/SpiralKernel.hpp"

namespace spiral {

SpiralKernel::SpiralKernel()
    : pressureRail_(router_, steamEngine_),
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

MindWheel& SpiralKernel::mindWheel() noexcept
{
    return mindWheel_;
}

CodingWheel& SpiralKernel::codingWheel() noexcept
{
    return codingWheel_;
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

bool SpiralKernel::selectMindPolicy(std::size_t notch)
{
    return mindWheel_.select(notch);
}

bool SpiralKernel::selectCodingAction(std::size_t notch)
{
    return codingWheel_.select(notch);
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
    if (signal) {
        // Disembark is where the transition re-enters the live Router Bus.
        pressureRail_.send(*signal);
    }
    return signal;
}

} // namespace spiral
