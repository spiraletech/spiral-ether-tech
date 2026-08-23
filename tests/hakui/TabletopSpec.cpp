#include <cassert>
#include <cstdint>
#include <memory>
#include <set>
#include <stdexcept>
#include <utility>

#include "games/BlackjackTable.hpp"
#include "games/CardDeck.hpp"
#include "games/Dice.hpp"
#include "games/GameTerminal.hpp"
#include "interaction/InteractionService.hpp"
#include "spiral/SpiralKernel.hpp"

namespace {

void deck_contains_exactly_52_unique_cards()
{
    hakui::games::CardDeck deck(52);
    std::set<std::pair<int, int>> cards;

    while (const auto card = deck.draw()) {
        cards.emplace(
            static_cast<int>(card->suit),
            static_cast<int>(card->rank)
        );
    }

    assert(cards.size() == 52);
    assert(deck.remaining() == 0);
    assert(deck.discarded() == 52);
    assert(!deck.draw().has_value());
}

void seeded_decks_and_dice_are_replayable()
{
    hakui::games::CardDeck leftDeck(7001);
    hakui::games::CardDeck rightDeck(7001);
    leftDeck.shuffle();
    rightDeck.shuffle();

    for (int index = 0; index < 52; ++index) {
        assert(leftDeck.draw() == rightDeck.draw());
    }

    hakui::games::DiceRoller leftDice(6);
    hakui::games::DiceRoller rightDice(6);
    assert(leftDice.roll(12, 20).values == rightDice.roll(12, 20).values);
}

void dice_validate_bounds_and_totals()
{
    hakui::games::DiceRoller dice(42);
    const auto result = dice.roll(4, 6);
    assert(result.values.size() == 4);
    assert(result.total >= 4 && result.total <= 24);

    bool rejected = false;
    try {
        (void)dice.roll(0, 6);
    } catch (const std::invalid_argument&) {
        rejected = true;
    }
    assert(rejected);
}

void blackjack_scores_aces_and_uses_virtual_credits()
{
    using hakui::games::Card;
    using hakui::games::Rank;
    using hakui::games::Suit;

    assert(hakui::games::BlackjackTable::handValue({
        Card{Suit::Spades, Rank::Ace},
        Card{Suit::Hearts, Rank::King}
    }) == 21);
    assert(hakui::games::BlackjackTable::handValue({
        Card{Suit::Spades, Rank::Ace},
        Card{Suit::Hearts, Rank::Ace},
        Card{Suit::Clubs, Rank::Nine}
    }) == 21);

    hakui::games::BlackjackTable table(1000, 99);
    table.grantCredits(50);
    assert(table.credits() == 1050);
    assert(!table.startRound(0));
    assert(!table.startRound(1051));
    assert(table.startRound(100));
    assert(table.credits() == 950 || table.phase() == hakui::games::BlackjackPhase::Settled);

    if (table.phase() == hakui::games::BlackjackPhase::PlayerTurn) {
        assert(table.stand());
    }
    assert(table.phase() == hakui::games::BlackjackPhase::Settled);
    assert(table.credits() >= 950);
}

void terminals_route_use_play_and_dice_through_interactions()
{
    spiral::SpiralKernel kernel;
    hakui::InteractionService interactions(kernel.router());
    auto terminal = std::make_shared<hakui::games::GameTerminal>(
        7001,
        hakui::games::TerminalModel::FusionDeck,
        7001
    );
    assert(interactions.registerTarget(terminal));
    assert(terminal->nextContextAction() ==
           hakui::games::TerminalContextAction::PowerOn);

    hakui::InteractionRequest request;
    request.actor = 1;
    request.target = 7001;
    request.verb = hakui::InteractionVerb::Use;
    assert(interactions.interact(request).handled);
    assert(terminal->powered());
    assert(terminal->virtualCredits() == 250);
    assert(terminal->nextContextAction() ==
           hakui::games::TerminalContextAction::OpenCards);

    request.verb = hakui::InteractionVerb::Play;
    assert(interactions.interact(request).handled);
    assert(terminal->activeApp() == hakui::games::TerminalApp::CardTable52);
    assert(terminal->nextContextAction() ==
           hakui::games::TerminalContextAction::Bet25);
    assert(terminal->beginCardRound(25));
    if (terminal->cardTable().phase() == hakui::games::BlackjackPhase::PlayerTurn) {
        const auto next = terminal->nextContextAction();
        assert(next == hakui::games::TerminalContextAction::Hit ||
               next == hakui::games::TerminalContextAction::Stand);
        assert(terminal->standCardTable());
    }
    assert(terminal->cardTable().phase() == hakui::games::BlackjackPhase::Settled);

    request.verb = hakui::InteractionVerb::Use;
    assert(interactions.interact(request).handled);
    assert(terminal->activeApp() == hakui::games::TerminalApp::DiceLab);
    assert(terminal->lastDiceResult().values.size() == 2);

    assert(kernel.stateStore().get("world.terminal.7001.powered") != nullptr);
    assert(kernel.stateStore().get("world.terminal.7001.dice.total") != nullptr);
}

} // namespace

int main()
{
    deck_contains_exactly_52_unique_cards();
    seeded_decks_and_dice_are_replayable();
    dice_validate_bounds_and_totals();
    blackjack_scores_aces_and_uses_virtual_credits();
    terminals_route_use_play_and_dice_through_interactions();
    return 0;
}
