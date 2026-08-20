#include "spiral/crystal/CrystalHost.hpp"

#include <algorithm>
#include <utility>

namespace spiral {

CrystalHost::CrystalHost(CrystalGrid& grid)
    : grid_(grid)
{
}

CrystalHost::~CrystalHost()
{
    unmountAll();
}

bool CrystalHost::mount(
    std::unique_ptr<CrystalModule> module,
    std::size_t x,
    std::size_t y
)
{
    if (!module) {
        return false;
    }

    Crystal& crystal = module->crystal();
    if (find(crystal.id()) != nullptr) {
        return false;
    }

    if (!grid_.attach(crystal, x, y)) {
        return false;
    }

    modules_.push_back(MountedModule{std::move(module), x, y});
    return true;
}

bool CrystalHost::unmount(CrystalId id)
{
    const auto it = std::find_if(
        modules_.begin(),
        modules_.end(),
        [id](const MountedModule& mounted) {
            return mounted.module && mounted.module->crystal().id() == id;
        }
    );

    if (it == modules_.end()) {
        return false;
    }

    // Grid detaches and returns non-dormant crystals before module destruction.
    if (!grid_.detach(id)) {
        return false;
    }

    modules_.erase(it);
    return true;
}

void CrystalHost::unmountAll()
{
    // Detach in reverse mount order so dependencies can be stacked naturally:
    // newest/outermost modules return before earlier/foundational modules.
    while (!modules_.empty()) {
        const CrystalId id = modules_.back().module->crystal().id();

        // Even if a future Grid implementation reports a detach failure, never
        // destroy the module while the Grid still references it. Stop here and
        // preserve ownership rather than creating a dangling pointer.
        if (!grid_.detach(id)) {
            return;
        }

        modules_.pop_back();
    }
}

CrystalModule* CrystalHost::find(CrystalId id) noexcept
{
    const auto it = std::find_if(
        modules_.begin(),
        modules_.end(),
        [id](const MountedModule& mounted) {
            return mounted.module && mounted.module->crystal().id() == id;
        }
    );

    return it == modules_.end() ? nullptr : it->module.get();
}

const CrystalModule* CrystalHost::find(CrystalId id) const noexcept
{
    const auto it = std::find_if(
        modules_.begin(),
        modules_.end(),
        [id](const MountedModule& mounted) {
            return mounted.module && mounted.module->crystal().id() == id;
        }
    );

    return it == modules_.end() ? nullptr : it->module.get();
}

bool CrystalHost::requestEmerge(CrystalId id)
{
    return find(id) != nullptr && grid_.requestEmerge(id);
}

bool CrystalHost::requestReturn(CrystalId id)
{
    return find(id) != nullptr && grid_.requestReturn(id);
}

std::size_t CrystalHost::size() const noexcept
{
    return modules_.size();
}

} // namespace spiral
