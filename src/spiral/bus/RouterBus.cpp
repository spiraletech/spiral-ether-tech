#include "spiral/bus/RouterBus.hpp"

#include <utility>
#include <vector>

namespace spiral {

RouterBus::ListenerId RouterBus::subscribe(Listener listener)
{
    const ListenerId id = nextListenerId_++;
    listeners_.emplace(id, std::move(listener));
    return id;
}

void RouterBus::unsubscribe(ListenerId id)
{
    listeners_.erase(id);
}

void RouterBus::emit(const Signal& signal) const
{
    // Snapshot listeners before dispatch. A listener may unsubscribe itself or
    // another listener while handling an event; that must not invalidate the
    // active iteration over the live registry.
    std::vector<Listener> snapshot;
    snapshot.reserve(listeners_.size());

    for (const auto& [id, listener] : listeners_) {
        (void)id;
        if (listener) {
            snapshot.push_back(listener);
        }
    }

    for (const Listener& listener : snapshot) {
        listener(signal);
    }
}

std::size_t RouterBus::listenerCount() const noexcept
{
    return listeners_.size();
}

} // namespace spiral
