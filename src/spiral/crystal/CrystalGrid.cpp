#include "spiral/crystal/CrystalGrid.hpp"

#include <algorithm>
#include <string>

namespace spiral {

CrystalGrid::CrystalGrid(RouterBus& bus, AUMField& field)
    : bus_(bus), field_(field)
{
    listenerId_ = bus_.subscribe([this](const Signal& signal) {
        route(signal);
    });
}

CrystalGrid::~CrystalGrid()
{
    if (listenerId_ != 0) {
        bus_.unsubscribe(listenerId_);
    }
}

bool CrystalGrid::attach(Crystal& crystal, std::size_t x, std::size_t y)
{
    if (findRecord(crystal.id()) != nullptr) {
        return false;
    }

    if (!field_.attachCrystal(x, y, crystal.id())) {
        return false;
    }

    records_.push_back(Record{&crystal, x, y, false, false});
    return true;
}

bool CrystalGrid::detach(CrystalId id)
{
    auto it = std::find_if(records_.begin(), records_.end(), [id](const Record& record) {
        return record.crystal && record.crystal->id() == id;
    });

    if (it == records_.end()) {
        return false;
    }

    if (it->crystal && it->crystal->state() != Crystal::State::Dormant) {
        it->crystal->returnToDormant();
    }

    field_.detachCrystal(it->x, it->y, id);
    records_.erase(it);
    return true;
}

bool CrystalGrid::requestEmerge(CrystalId id)
{
    Record* record = findRecord(id);
    if (!record) {
        return false;
    }

    record->emergeRequested = true;
    record->returnRequested = false;
    return true;
}

bool CrystalGrid::requestReturn(CrystalId id)
{
    Record* record = findRecord(id);
    if (!record) {
        return false;
    }

    record->returnRequested = true;
    record->emergeRequested = false;
    return true;
}

void CrystalGrid::tick(float dtSeconds, AUMPhase phase)
{
    for (Record& record : records_) {
        if (!record.crystal) {
            continue;
        }

        switch (phase) {
            case AUMPhase::A_Emergence:
                if (record.emergeRequested && record.crystal->state() == Crystal::State::Dormant) {
                    record.emergeRequested = false;
                    if (!record.crystal->emerge(bus_)) {
                        emitCrystalError(*record.crystal, "emergence failed; crystal isolated");
                    }
                }
                break;

            case AUMPhase::U_Sustain:
                if (record.crystal->healthy() &&
                    record.crystal->state() == Crystal::State::Sustaining) {
                    record.crystal->sustain(dtSeconds);
                }
                break;

            case AUMPhase::M_Return:
                if (record.returnRequested &&
                    record.crystal->state() != Crystal::State::Dormant) {
                    record.returnRequested = false;
                    record.crystal->returnToDormant();
                }
                break;
        }
    }
}

Crystal* CrystalGrid::find(CrystalId id) noexcept
{
    Record* record = findRecord(id);
    return record ? record->crystal : nullptr;
}

const Crystal* CrystalGrid::find(CrystalId id) const noexcept
{
    const Record* record = findRecord(id);
    return record ? record->crystal : nullptr;
}

CrystalGrid::Record* CrystalGrid::findRecord(CrystalId id) noexcept
{
    const auto it = std::find_if(records_.begin(), records_.end(), [id](const Record& record) {
        return record.crystal && record.crystal->id() == id;
    });
    return it == records_.end() ? nullptr : &(*it);
}

const CrystalGrid::Record* CrystalGrid::findRecord(CrystalId id) const noexcept
{
    const auto it = std::find_if(records_.begin(), records_.end(), [id](const Record& record) {
        return record.crystal && record.crystal->id() == id;
    });
    return it == records_.end() ? nullptr : &(*it);
}

void CrystalGrid::route(const Signal& signal)
{
    if (signal.destination.empty()) {
        return;
    }

    for (Record& record : records_) {
        if (!record.crystal) {
            continue;
        }

        if (signal.destination == record.crystal->name()) {
            record.crystal->onSignal(signal);
        }
    }
}

void CrystalGrid::emitCrystalError(const Crystal& crystal, const char* message)
{
    Signal signal;
    signal.kind = SignalKind::Error;
    signal.source = std::string(crystal.name());
    signal.destination = "spiral.core";
    signal.topic = "crystal.error";
    signal.payload = message;
    signal.timestamp = Clock::now();
    bus_.emit(signal);
}

} // namespace spiral
