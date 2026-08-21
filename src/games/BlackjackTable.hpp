#pragma once

#include <cstdint>
#include <random>
#include <vector>

#include "games/CardDeck.hpp"

namespace hakui::games {

enum class BlackjackPhase { WaitingForWager, PlayerTurn, Settled };
enum class BlackjackResult { None, PlayerBlackjack, PlayerWin, DealerWin, Push };

class BlackjackTable {
public:
    explicit BlackjackTable(
        std::int64_t initialCredits = 1000,
        std::uint32_t seed = std::random_device{}()
    );

    bool startRound(std::int64_t wager);
    bool hit();
    bool stand();
    void grantCredits(std::int64_t amount) noexcept;

    static int handValue(const std::vector<Card>& hand) noexcept;

    BlackjackPhase phase() const noexcept;
    BlackjackResult result() const noexcept;
    std::int64_t credits() const noexcept;
    std::int64_t wager() const noexcept;
    const std::vector<Card>& playerHand() const noexcept;
    const std::vector<Card>& dealerHand() const noexcept;

private:
    bool dealTo(std::vector<Card>& hand);
    void resolveNaturals();
    void settle(BlackjackResult result);

    CardDeck deck_;
    std::vector<Card> playerHand_;
    std::vector<Card> dealerHand_;
    BlackjackPhase phase_ = BlackjackPhase::WaitingForWager;
    BlackjackResult result_ = BlackjackResult::None;
    std::int64_t credits_ = 0;
    std::int64_t wager_ = 0;
};

} // namespace hakui::games
