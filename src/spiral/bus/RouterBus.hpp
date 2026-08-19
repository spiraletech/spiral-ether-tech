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

    // Plumbing only: distribute the packet exactly as received.
    // No policy, no action selection, no AUM interpretation.
    void emit(const Signal& signal) const;

    std::size_t listenerCount() const noexcept;

private:
    ListenerId nextListenerId_ = 1;
    std::unordered_map<ListenerId, Listener> listeners_;
};

} // namespace spiral
