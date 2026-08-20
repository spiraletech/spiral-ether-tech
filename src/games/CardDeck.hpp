#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <random>
#include <vector>

#include "games/Card.hpp"

namespace hakui::games {

class CardDeck {
public:
    explicit CardDeck(std::uint32_t seed = std::random_device{}());

    void reset();
    void shuffle();
    std::optional<Card> draw();

    std::size_t remaining() const noexcept;
    std::size_t discarded() const noexcept;

private:
    std::mt19937 random_;
    std::vector<Card> cards_;
    std::size_t next_ = 0;
};

} // namespace hakui::games
