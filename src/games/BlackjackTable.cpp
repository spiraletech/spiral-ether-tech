#include "games/BlackjackTable.hpp"

#include <algorithm>

namespace hakui::games {

BlackjackTable::BlackjackTable(std::int64_t initialCredits, std::uint32_t seed)
    : deck_(seed),
      credits_(std::max<std::int64_t>(initialCredits, 0))
{
    deck_.shuffle();
}

bool BlackjackTable::startRound(std::int64_t wager)
{
    if (phase_ == BlackjackPhase::PlayerTurn || wager <= 0 || wager > credits_) {
        return false;
    }

    if (deck_.remaining() < 15) {
        deck_.reset();
        deck_.shuffle();
    }

    playerHand_.clear();
    dealerHand_.clear();
    result_ = BlackjackResult::None;
    wager_ = wager;
    credits_ -= wager_;
    phase_ = BlackjackPhase::PlayerTurn;

    if (!dealTo(playerHand_) || !dealTo(dealerHand_) ||
        !dealTo(playerHand_) || !dealTo(dealerHand_)) {
        credits_ += wager_;
        wager_ = 0;
        phase_ = BlackjackPhase::WaitingForWager;
        return false;
    }

    resolveNaturals();
    return true;
}

bool BlackjackTable::hit()
{
    if (phase_ != BlackjackPhase::PlayerTurn || !dealTo(playerHand_)) {
        return false;
    }

    if (handValue(playerHand_) > 21) {
        settle(BlackjackResult::DealerWin);
    }
    return true;
}

bool BlackjackTable::stand()
{
    if (phase_ != BlackjackPhase::PlayerTurn) {
        return false;
    }

    while (handValue(dealerHand_) < 17) {
        if (!dealTo(dealerHand_)) {
            return false;
        }
    }

    const int player = handValue(playerHand_);
    const int dealer = handValue(dealerHand_);
    if (dealer > 21 || player > dealer) {
        settle(BlackjackResult::PlayerWin);
    } else if (player < dealer) {
        settle(BlackjackResult::DealerWin);
    } else {
        settle(BlackjackResult::Push);
    }
    return true;
}

void BlackjackTable::grantCredits(std::int64_t amount) noexcept
{
    if (amount > 0) {
        credits_ += amount;
    }
}

int BlackjackTable::handValue(const std::vector<Card>& hand) noexcept
{
    int value = 0;
    int aces = 0;

    for (const Card& card : hand) {
        const int rank = static_cast<int>(card.rank);
        if (card.rank == Rank::Ace) {
            value += 11;
            ++aces;
        } else {
            value += std::min(rank, 10);
        }
    }

    while (value > 21 && aces > 0) {
        value -= 10;
        --aces;
    }
    return value;
}

BlackjackPhase BlackjackTable::phase() const noexcept { return phase_; }
BlackjackResult BlackjackTable::result() const noexcept { return result_; }
std::int64_t BlackjackTable::credits() const noexcept { return credits_; }
std::int64_t BlackjackTable::wager() const noexcept { return wager_; }
const std::vector<Card>& BlackjackTable::playerHand() const noexcept { return playerHand_; }
const std::vector<Card>& BlackjackTable::dealerHand() const noexcept { return dealerHand_; }

bool BlackjackTable::dealTo(std::vector<Card>& hand)
{
    const auto card = deck_.draw();
    if (!card) {
        return false;
    }
    hand.push_back(*card);
    return true;
}

void BlackjackTable::resolveNaturals()
{
    const bool playerBlackjack = handValue(playerHand_) == 21;
    const bool dealerBlackjack = handValue(dealerHand_) == 21;

    if (playerBlackjack && dealerBlackjack) {
        settle(BlackjackResult::Push);
    } else if (playerBlackjack) {
        settle(BlackjackResult::PlayerBlackjack);
    } else if (dealerBlackjack) {
        settle(BlackjackResult::DealerWin);
    }
}

void BlackjackTable::settle(BlackjackResult result)
{
    result_ = result;
    phase_ = BlackjackPhase::Settled;

    switch (result) {
        case BlackjackResult::PlayerBlackjack:
            credits_ += wager_ + (wager_ * 3) / 2;
            break;
        case BlackjackResult::PlayerWin:
            credits_ += wager_ * 2;
            break;
        case BlackjackResult::Push:
            credits_ += wager_;
            break;
        case BlackjackResult::DealerWin:
        case BlackjackResult::None:
            break;
    }
}

} // namespace hakui::games
