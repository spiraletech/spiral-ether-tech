#pragma once

#include <cstdint>
#include <random>
#include <vector>

namespace hakui::games {

struct DiceResult {
    std::vector<std::uint16_t> values;
    std::uint32_t total = 0;
};

class DiceRoller {
public:
    explicit DiceRoller(std::uint32_t seed = std::random_device{}());

    DiceResult roll(std::uint16_t count, std::uint16_t sides);

private:
    std::mt19937 random_;
};

} // namespace hakui::games
