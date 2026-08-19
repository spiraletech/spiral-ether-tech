#pragma once

#include <optional>

#include "spiral/core/SpiralTypes.hpp"

namespace spiral {

// Ether Bus is not a fast queue. It is the explicit transition space between
// modes/actions. Transit cannot be skipped and arrival requires Disembark.
class EtherBus {
public:
    enum class State {
        Idle,
        Boarding,
        Transit,
        ReadyToDisembark
    };

    static constexpr Milliseconds kMinimumTransitFloor{2000};
    static constexpr Milliseconds kDefaultTransit{2500};

    explicit EtherBus(Milliseconds transit = kDefaultTransit);

    bool board(const Signal& signal, TimePoint now = Clock::now());
    void tick(TimePoint now = Clock::now());

    // Explicit release gate. There is intentionally no skip() API.
    std::optional<Signal> disembark();

    State state() const noexcept;
    bool occupied() const noexcept;
    bool ready() const noexcept;
    Milliseconds transitDuration() const noexcept;

private:
    State state_ = State::Idle;
    Milliseconds transitDuration_ = kDefaultTransit;
    TimePoint boardedAt_{};
    std::optional<Signal> passenger_;
};

} // namespace spiral
