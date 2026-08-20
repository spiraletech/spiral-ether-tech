#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>

#include "spiral/bus/RouterBus.hpp"

namespace spiral {

// Session-state reducer recovered from the older Spiral patch/merge pattern.
//
// Live mutations enter through Router Bus `State` signals. The store merges
// each patch into the existing key/value map and increments one revision per
// applied state event. Persistence formats remain outside Core; snapshot/restore
// provide the adapter boundary.
class StateStore {
public:
    struct Snapshot {
        std::uint64_t revision = 0;
        SignalId lastSignalId = 0;
        std::unordered_map<std::string, StateValue> values;
    };

    explicit StateStore(RouterBus& bus);
    ~StateStore();

    StateStore(const StateStore&) = delete;
    StateStore& operator=(const StateStore&) = delete;

    bool contains(std::string_view key) const;
    const StateValue* get(std::string_view key) const;

    std::uint64_t revision() const noexcept;
    SignalId lastSignalId() const noexcept;
    std::size_t size() const noexcept;

    Snapshot snapshot() const;

    // Restore is for an outer persistence adapter during boot/load. Runtime
    // state changes should enter through Router Bus instead of calling restore.
    void restore(Snapshot snapshot);
    void clear();

private:
    void onSignal(const Signal& signal);
    void applyPatch(const StatePatch& patch, SignalId signalId);

private:
    RouterBus& bus_;
    RouterBus::ListenerId listenerId_ = 0;
    std::uint64_t revision_ = 0;
    SignalId lastSignalId_ = 0;
    std::unordered_map<std::string, StateValue> values_;
};

} // namespace spiral
