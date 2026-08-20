#pragma once

#include <cstddef>
#include <optional>
#include <vector>

#include "spiral/aum/AUMField.hpp"
#include "spiral/bus/RouterBus.hpp"
#include "spiral/crystal/Crystal.hpp"

namespace spiral {

// Coordinates independent capability crystals across the AUM field.
// A failed crystal is contained here; it is never a Hakui-core boot failure.
class CrystalGrid {
public:
    CrystalGrid(RouterBus& bus, AUMField& field);
    ~CrystalGrid();

    bool attach(Crystal& crystal, std::size_t x, std::size_t y);
    bool detach(CrystalId id);

    bool requestEmerge(CrystalId id);
    bool requestReturn(CrystalId id);

    void tick(float dtSeconds, AUMPhase phase);

    Crystal* find(CrystalId id) noexcept;
    const Crystal* find(CrystalId id) const noexcept;

private:
    struct Record {
        Crystal* crystal = nullptr;
        std::size_t x = 0;
        std::size_t y = 0;
        bool emergeRequested = false;
        bool returnRequested = false;
    };

    Record* findRecord(CrystalId id) noexcept;
    const Record* findRecord(CrystalId id) const noexcept;
    void route(const Signal& signal);
    void emitCrystalError(const Crystal& crystal, const char* message);

private:
    RouterBus& bus_;
    AUMField& field_;
    RouterBus::ListenerId listenerId_ = 0;
    std::vector<Record> records_;
};

} // namespace spiral
