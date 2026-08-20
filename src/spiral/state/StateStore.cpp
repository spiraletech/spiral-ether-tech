#include "spiral/state/StateStore.hpp"

#include <utility>

namespace spiral {

StateStore::StateStore(RouterBus& bus)
    : bus_(bus)
{
    listenerId_ = bus_.subscribe([this](const Signal& signal) {
        onSignal(signal);
    });
}

StateStore::~StateStore()
{
    if (listenerId_ != 0) {
        bus_.unsubscribe(listenerId_);
    }
}

bool StateStore::contains(std::string_view key) const
{
    return values_.find(std::string(key)) != values_.end();
}

const StateValue* StateStore::get(std::string_view key) const
{
    const auto it = values_.find(std::string(key));
    return it == values_.end() ? nullptr : &it->second;
}

std::uint64_t StateStore::revision() const noexcept
{
    return revision_;
}

SignalId StateStore::lastSignalId() const noexcept
{
    return lastSignalId_;
}

std::size_t StateStore::size() const noexcept
{
    return values_.size();
}

StateStore::Snapshot StateStore::snapshot() const
{
    Snapshot out;
    out.revision = revision_;
    out.lastSignalId = lastSignalId_;
    out.values = values_;
    return out;
}

void StateStore::restore(Snapshot snapshot)
{
    revision_ = snapshot.revision;
    lastSignalId_ = snapshot.lastSignalId;
    values_ = std::move(snapshot.values);
}

void StateStore::clear()
{
    revision_ = 0;
    lastSignalId_ = 0;
    values_.clear();
}

void StateStore::onSignal(const Signal& signal)
{
    if (signal.kind != SignalKind::State || signal.statePatch.empty()) {
        return;
    }

    applyPatch(signal.statePatch, signal.id);
}

void StateStore::applyPatch(const StatePatch& patch, SignalId signalId)
{
    bool applied = false;

    for (const StatePatchEntry& entry : patch) {
        if (entry.key.empty()) {
            continue;
        }

        values_[entry.key] = entry.value;
        applied = true;
    }

    if (!applied) {
        return;
    }

    ++revision_;
    lastSignalId_ = signalId;
}

} // namespace spiral
