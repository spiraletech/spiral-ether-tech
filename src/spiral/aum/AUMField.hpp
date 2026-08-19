#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "spiral/core/SpiralTypes.hpp"

namespace spiral {

class AUMField {
public:
    struct Node {
        std::size_t x = 0;
        std::size_t y = 0;
        bool core = false;
        std::vector<CrystalId> crystals;
    };

    static constexpr float kPhaseSeconds = 3.0f;
    static constexpr float kCycleSeconds = 9.0f;

    AUMField();

    void tick(float dtSeconds);

    AUMPhase phase() const noexcept;
    float cycleTime() const noexcept;

    bool attachCrystal(std::size_t x, std::size_t y, CrystalId id);
    bool detachCrystal(std::size_t x, std::size_t y, CrystalId id);

    const Node* node(std::size_t x, std::size_t y) const noexcept;
    const Node& coreNode() const noexcept;

private:
    static constexpr std::size_t indexOf(std::size_t x, std::size_t y)
    {
        return y * kAUMFieldSize + x;
    }

private:
    std::array<Node, kAUMFieldSize * kAUMFieldSize> nodes_{};
    float cycleTime_ = 0.0f;
    AUMPhase phase_ = AUMPhase::A_Emergence;
};

} // namespace spiral
