#pragma once

#include <cstddef>
#include <deque>
#include <vector>

#include "spiral/bus/RouterBus.hpp"

namespace spiral {

// Passive audit store recovered from Spiral OS' MONOLITH Store.
// It listens to every Router Bus signal and retains only the newest N entries.
class MonolithLedger {
public:
    static constexpr std::size_t kDefaultCapacity = 1000;

    explicit MonolithLedger(
        RouterBus& bus,
        std::size_t capacity = kDefaultCapacity
    );

    ~MonolithLedger();

    MonolithLedger(const MonolithLedger&) = delete;
    MonolithLedger& operator=(const MonolithLedger&) = delete;

    std::size_t size() const noexcept;
    std::size_t capacity() const noexcept;
    bool empty() const noexcept;

    std::vector<Signal> snapshot() const;
    std::vector<Signal> tail(std::size_t count) const;

    void clear();

private:
    void record(const Signal& signal);

private:
    RouterBus& bus_;
    RouterBus::ListenerId listenerId_ = 0;
    std::size_t capacity_ = kDefaultCapacity;
    std::deque<Signal> events_;
};

} // namespace spiral
