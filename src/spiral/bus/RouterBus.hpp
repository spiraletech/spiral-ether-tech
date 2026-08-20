#pragma once

#include <cstddef>
#include <functional>
#include <unordered_map>
#include <vector>

#include "spiral/core/SpiralTypes.hpp"

namespace spiral {

class RouterBus {
public:
    using ListenerId = std::uint64_t;
    using Listener = std::function<void(const Signal&)>;

    ListenerId subscribe(Listener listener);
    void unsubscribe(ListenerId id);

    // Plumbing only: sequence and distribute the packet.
    // No policy, no action selection, no AUM interpretation.
    // A zero signal id is assigned here so event identity belongs to the bus,
    // not to individual crystals/programs.
    SignalId emit(Signal signal);

    std::size_t listenerCount() const noexcept;
    SignalId nextSignalId() const noexcept;

private:
    ListenerId nextListenerId_ = 1;
    SignalId nextSignalId_ = 1;
    std::unordered_map<ListenerId, Listener> listeners_;
};

} // namespace spiral
