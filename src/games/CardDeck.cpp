#include "games/CardDeck.hpp"

#include <algorithm>

namespace hakui::games {

CardDeck::CardDeck(std::uint32_t seed)
    : random_(seed)
{
    reset();
}

void CardDeck::reset()
{
    cards_.clear();
    cards_.reserve(52);

    for (int suit = static_cast<int>(Suit::Clubs);
         suit <= static_cast<int>(Suit::Spades);
         ++suit) {
        for (int rank = static_cast<int>(Rank::Two);
             rank <= static_cast<int>(Rank::Ace);
             ++rank) {
            cards_.push_back(Card{
                static_cast<Suit>(suit),
                static_cast<Rank>(rank)
            });
        }
    }

    next_ = 0;
}

void CardDeck::shuffle()
{
    std::shuffle(cards_.begin(), cards_.end(), random_);
    next_ = 0;
}

std::optional<Card> CardDeck::draw()
{
    if (next_ >= cards_.size()) {
        return std::nullopt;
    }
    return cards_[next_++];
}

std::size_t CardDeck::remaining() const noexcept
{
    return cards_.size() - next_;
}

std::size_t CardDeck::discarded() const noexcept
{
    return next_;
}

} // namespace hakui::games
