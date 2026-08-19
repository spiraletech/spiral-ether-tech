#include "spiral/aum/AUMField.hpp"

#include <algorithm>
#include <cmath>

namespace spiral {

AUMField::AUMField()
{
    for (std::size_t y = 0; y < kAUMFieldSize; ++y) {
        for (std::size_t x = 0; x < kAUMFieldSize; ++x) {
            Node& n = nodes_[indexOf(x, y)];
            n.x = x;
            n.y = y;
            n.core = (x == kAUMCenter && y == kAUMCenter);
        }
    }
}

void AUMField::tick(float dtSeconds)
{
    if (dtSeconds <= 0.0f) {
        return;
    }

    cycleTime_ = std::fmod(cycleTime_ + dtSeconds, kCycleSeconds);

    if (cycleTime_ < kPhaseSeconds) {
        phase_ = AUMPhase::A_Emergence;
    } else if (cycleTime_ < kPhaseSeconds * 2.0f) {
        phase_ = AUMPhase::U_Sustain;
    } else {
        phase_ = AUMPhase::M_Return;
    }
}

AUMPhase AUMField::phase() const noexcept
{
    return phase_;
}

float AUMField::cycleTime() const noexcept
{
    return cycleTime_;
}

bool AUMField::attachCrystal(std::size_t x, std::size_t y, CrystalId id)
{
    if (x >= kAUMFieldSize || y >= kAUMFieldSize) {
        return false;
    }

    auto& crystals = nodes_[indexOf(x, y)].crystals;
    if (std::find(crystals.begin(), crystals.end(), id) != crystals.end()) {
        return true;
    }

    crystals.push_back(id);
    return true;
}

bool AUMField::detachCrystal(std::size_t x, std::size_t y, CrystalId id)
{
    if (x >= kAUMFieldSize || y >= kAUMFieldSize) {
        return false;
    }

    auto& crystals = nodes_[indexOf(x, y)].crystals;
    const auto it = std::remove(crystals.begin(), crystals.end(), id);
    const bool removed = it != crystals.end();
    crystals.erase(it, crystals.end());
    return removed;
}

const AUMField::Node* AUMField::node(std::size_t x, std::size_t y) const noexcept
{
    if (x >= kAUMFieldSize || y >= kAUMFieldSize) {
        return nullptr;
    }
    return &nodes_[indexOf(x, y)];
}

const AUMField::Node& AUMField::coreNode() const noexcept
{
    return nodes_[indexOf(kAUMCenter, kAUMCenter)];
}

} // namespace spiral
