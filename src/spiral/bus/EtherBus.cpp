#include "spiral/bus/EtherBus.hpp"

#include <algorithm>

namespace spiral {

EtherBus::EtherBus(Milliseconds transit)
    : transitDuration_(std::max(transit, kMinimumTransitFloor))
{
}

bool EtherBus::board(const Signal& signal, TimePoint now)
{
    if (state_ != State::Idle || passenger_.has_value()) {
        return false;
    }

    passenger_ = signal;
    boardedAt_ = now;
    state_ = State::Boarding;
    return true;
}

void EtherBus::tick(TimePoint now)
{
    if (state_ == State::Idle || !passenger_) {
        return;
    }

    if (state_ == State::Boarding) {
        state_ = State::Transit;
    }

    if (state_ == State::Transit && now - boardedAt_ >= transitDuration_) {
        state_ = State::ReadyToDisembark;
    }
}

std::optional<Signal> EtherBus::disembark()
{
    if (state_ != State::ReadyToDisembark || !passenger_) {
        return std::nullopt;
    }

    std::optional<Signal> out = std::move(passenger_);
    passenger_.reset();
    state_ = State::Idle;
    return out;
}

EtherBus::State EtherBus::state() const noexcept
{
    return state_;
}

bool EtherBus::occupied() const noexcept
{
    return passenger_.has_value();
}

bool EtherBus::ready() const noexcept
{
    return state_ == State::ReadyToDisembark;
}

Milliseconds EtherBus::transitDuration() const noexcept
{
    return transitDuration_;
}

} // namespace spiral
