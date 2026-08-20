#include "spiral/ledger/MonolithLedger.hpp"

#include <algorithm>

namespace spiral {

MonolithLedger::MonolithLedger(RouterBus& bus, std::size_t capacity)
    : bus_(bus),
      capacity_(std::max<std::size_t>(1, capacity))
{
    listenerId_ = bus_.subscribe([this](const Signal& signal) {
        record(signal);
    });
}

MonolithLedger::~MonolithLedger()
{
    if (listenerId_ != 0) {
        bus_.unsubscribe(listenerId_);
    }
}

std::size_t MonolithLedger::size() const noexcept
{
    return events_.size();
}

std::size_t MonolithLedger::capacity() const noexcept
{
    return capacity_;
}

bool MonolithLedger::empty() const noexcept
{
    return events_.empty();
}

std::vector<Signal> MonolithLedger::snapshot() const
{
    return std::vector<Signal>(events_.begin(), events_.end());
}

std::vector<Signal> MonolithLedger::tail(std::size_t count) const
{
    count = std::min(count, events_.size());
    const auto first = events_.end() - static_cast<std::ptrdiff_t>(count);
    return std::vector<Signal>(first, events_.end());
}

void MonolithLedger::clear()
{
    events_.clear();
}

void MonolithLedger::record(const Signal& signal)
{
    events_.push_back(signal);
    while (events_.size() > capacity_) {
        events_.pop_front();
    }
}

} // namespace spiral
