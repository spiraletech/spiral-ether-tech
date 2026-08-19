#include "spiral/crystal/ImvuSkeletonCrystal.hpp"

#include <utility>

#include "spiral/bus/RouterBus.hpp"

namespace spiral {

ImvuSkeletonCrystal::ImvuSkeletonCrystal(BackendHooks hooks)
    : hooks_(std::move(hooks))
{
}

CrystalId ImvuSkeletonCrystal::id() const noexcept
{
    return kId;
}

std::string_view ImvuSkeletonCrystal::name() const noexcept
{
    return "crystal.imvu_skeleton";
}

bool ImvuSkeletonCrystal::emerge(RouterBus& bus)
{
    (void)bus;

    if (state_ == State::Sustaining) {
        return true;
    }

    state_ = State::Emerging;

    if (!hooks_.available || !hooks_.available()) {
        state_ = State::Faulted;
        return false;
    }

    if (!hooks_.boot || !hooks_.boot()) {
        state_ = State::Faulted;
        return false;
    }

    state_ = State::Sustaining;
    return true;
}

void ImvuSkeletonCrystal::sustain(float dtSeconds)
{
    if (state_ != State::Sustaining) {
        return;
    }

    if (hooks_.tick) {
        hooks_.tick(dtSeconds);
    }
}

void ImvuSkeletonCrystal::returnToDormant()
{
    if (state_ == State::Dormant) {
        return;
    }

    state_ = State::Returning;

    if (hooks_.shutdown) {
        hooks_.shutdown();
    }

    state_ = State::Dormant;
}

void ImvuSkeletonCrystal::onSignal(const Signal& signal)
{
    if (state_ != State::Sustaining) {
        return;
    }

    if (hooks_.onSignal) {
        hooks_.onSignal(signal);
    }
}

Crystal::State ImvuSkeletonCrystal::state() const noexcept
{
    return state_;
}

bool ImvuSkeletonCrystal::healthy() const noexcept
{
    return state_ == State::Dormant || state_ == State::Sustaining;
}

} // namespace spiral
