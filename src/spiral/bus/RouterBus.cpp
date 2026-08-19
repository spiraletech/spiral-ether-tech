#include "spiral/bus/RouterBus.hpp"

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
    for (const auto& [id, listener] : listeners_) {
        (void)id;
        if (listener) {
            listener(signal);
        }
    }
}

std::size_t RouterBus::listenerCount() const noexcept
{
    return listeners_.size();
}

} // namespace spiral
