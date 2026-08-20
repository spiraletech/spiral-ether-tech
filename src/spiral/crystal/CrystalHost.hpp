#pragma once

#include <cstddef>
#include <memory>
#include <vector>

#include "spiral/crystal/CrystalGrid.hpp"
#include "spiral/crystal/CrystalModule.hpp"

namespace spiral {

// Owns mounted capability modules while CrystalGrid owns only their spatial /
// lifecycle relationships. Modules are detached before their memory dies.
class CrystalHost {
public:
    explicit CrystalHost(CrystalGrid& grid);
    ~CrystalHost();

    CrystalHost(const CrystalHost&) = delete;
    CrystalHost& operator=(const CrystalHost&) = delete;

    bool mount(
        std::unique_ptr<CrystalModule> module,
        std::size_t x,
        std::size_t y
    );

    bool unmount(CrystalId id);
    void unmountAll();

    CrystalModule* find(CrystalId id) noexcept;
    const CrystalModule* find(CrystalId id) const noexcept;

    bool requestEmerge(CrystalId id);
    bool requestReturn(CrystalId id);

    std::size_t size() const noexcept;

private:
    struct MountedModule {
        std::unique_ptr<CrystalModule> module;
        std::size_t x = 0;
        std::size_t y = 0;
    };

private:
    CrystalGrid& grid_;
    std::vector<MountedModule> modules_;
};

} // namespace spiral
