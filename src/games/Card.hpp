#pragma once

#include <cstdint>
#include <string_view>

namespace hakui::games {

enum class Suit : std::uint8_t { Clubs, Diamonds, Hearts, Spades };
enum class Rank : std::uint8_t {
    Two = 2,
    Three,
    Four,
    Five,
    Six,
    Seven,
    Eight,
    Nine,
    Ten,
    Jack,
    Queen,
    King,
    Ace
};

struct Card {
    Suit suit = Suit::Clubs;
    Rank rank = Rank::Two;

    friend bool operator==(const Card&, const Card&) = default;
};

constexpr std::string_view suitName(Suit suit) noexcept
{
    switch (suit) {
        case Suit::Clubs: return "clubs";
        case Suit::Diamonds: return "diamonds";
        case Suit::Hearts: return "hearts";
        case Suit::Spades: return "spades";
    }
    return "unknown";
}

constexpr std::string_view rankName(Rank rank) noexcept
{
    switch (rank) {
        case Rank::Two: return "two";
        case Rank::Three: return "three";
        case Rank::Four: return "four";
        case Rank::Five: return "five";
        case Rank::Six: return "six";
        case Rank::Seven: return "seven";
        case Rank::Eight: return "eight";
        case Rank::Nine: return "nine";
        case Rank::Ten: return "ten";
        case Rank::Jack: return "jack";
        case Rank::Queen: return "queen";
        case Rank::King: return "king";
        case Rank::Ace: return "ace";
    }
    return "unknown";
}

} // namespace hakui::games
