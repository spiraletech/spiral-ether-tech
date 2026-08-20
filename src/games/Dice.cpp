#include "games/Dice.hpp"

#include <stdexcept>

namespace hakui::games {

DiceRoller::DiceRoller(std::uint32_t seed)
    : random_(seed)
{
}

DiceResult DiceRoller::roll(std::uint16_t count, std::uint16_t sides)
{
    if (count == 0 || count > 100) {
        throw std::invalid_argument("dice count must be between 1 and 100");
    }
    if (sides < 2 || sides > 1000) {
        throw std::invalid_argument("die sides must be between 2 and 1000");
    }

    std::uniform_int_distribution<std::uint16_t> distribution(1, sides);
    DiceResult result;
    result.values.reserve(count);

    for (std::uint16_t index = 0; index < count; ++index) {
        const std::uint16_t value = distribution(random_);
        result.values.push_back(value);
        result.total += value;
    }

    return result;
}

} // namespace hakui::games
