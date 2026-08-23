#include "games/GameTerminal.hpp"

#include <string>

namespace hakui::games {

GameTerminal::GameTerminal(
    hakui::EntityId id,
    TerminalModel model,
    std::uint32_t seed
)
    : id_(id),
      model_(model),
      cardTable_(250, seed),
      dice_(seed ^ 0xD1CEU)
{
}

hakui::EntityId GameTerminal::interactionId() const noexcept
{
    return id_;
}

std::vector<hakui::InteractionOption> GameTerminal::interactionOptions(
    hakui::EntityId actor
) const
{
    (void)actor;
    if (!powered_) {
        return {
            {hakui::InteractionVerb::Use, "Power on terminal"},
            {hakui::InteractionVerb::Inspect, "Inspect terminal"}
        };
    }
    return {
        {hakui::InteractionVerb::Play, "Open tabletop suite"},
        {hakui::InteractionVerb::Use, "Roll two dice"},
        {hakui::InteractionVerb::Inspect, "Inspect terminal"}
    };
}

hakui::InteractionResult GameTerminal::interact(
    const hakui::InteractionRequest& request
)
{
    hakui::InteractionResult result;

    if (request.verb == hakui::InteractionVerb::Inspect) {
        result.handled = true;
        result.output = std::string(terminalModelName(model_)) +
            (powered_ ? " online" : " offline");
        return result;
    }

    if (request.verb == hakui::InteractionVerb::Use && !powered_) {
        powered_ = true;
        activeApp_ = TerminalApp::Home;
        result.handled = true;
        result.output = "terminal online";
        result.statePatch = {
            {"world.terminal." + std::to_string(id_) + ".powered", true},
            {"world.terminal." + std::to_string(id_) + ".model",
             std::string(terminalModelName(model_))}
        };
        return result;
    }

    if (request.verb == hakui::InteractionVerb::Play && powered_) {
        activeApp_ = TerminalApp::CardTable52;
        result.handled = true;
        result.output = "card table 52 ready; virtual credits only";
        result.statePatch = {
            {"world.terminal." + std::to_string(id_) + ".app",
             std::string("card_table_52")}
        };
        return result;
    }

    if (request.verb == hakui::InteractionVerb::Use && powered_) {
        activeApp_ = TerminalApp::DiceLab;
        lastDice_ = dice_.roll(2, 6);
        lastDiceReward_ = 0;
        if (lastDice_.values.size() == 2 &&
            lastDice_.values[0] == lastDice_.values[1]) {
            lastDiceReward_ = static_cast<std::int64_t>(lastDice_.total * 5);
            cardTable_.grantCredits(lastDiceReward_);
        }
        result.handled = true;
        result.output = "dice total=" + std::to_string(lastDice_.total);
        if (lastDiceReward_ > 0) {
            result.output += " // doubles reward=" + std::to_string(lastDiceReward_);
        }
        result.statePatch = {
            {"world.terminal." + std::to_string(id_) + ".app",
             std::string("dice_lab")},
            {"world.terminal." + std::to_string(id_) + ".dice.total",
             static_cast<std::int64_t>(lastDice_.total)},
            {"world.terminal." + std::to_string(id_) + ".credits",
             cardTable_.credits()}
        };
        return result;
    }

    return result;
}

bool GameTerminal::powered() const noexcept { return powered_; }
TerminalApp GameTerminal::activeApp() const noexcept { return activeApp_; }

bool GameTerminal::beginCardRound(std::int64_t virtualWager)
{
    return powered_ && activeApp_ == TerminalApp::CardTable52 &&
           cardTable_.startRound(virtualWager);
}

bool GameTerminal::hitCardTable()
{
    return powered_ && activeApp_ == TerminalApp::CardTable52 &&
           cardTable_.hit();
}

bool GameTerminal::standCardTable()
{
    return powered_ && activeApp_ == TerminalApp::CardTable52 &&
           cardTable_.stand();
}

const BlackjackTable& GameTerminal::cardTable() const noexcept { return cardTable_; }
DiceResult GameTerminal::lastDiceResult() const { return lastDice_; }
std::int64_t GameTerminal::virtualCredits() const noexcept { return cardTable_.credits(); }
std::int64_t GameTerminal::lastDiceReward() const noexcept { return lastDiceReward_; }

TerminalContextAction GameTerminal::nextContextAction() const noexcept
{
    if (!powered_) {
        return TerminalContextAction::PowerOn;
    }
    if (activeApp_ != TerminalApp::CardTable52) {
        return TerminalContextAction::OpenCards;
    }
    if (cardTable_.phase() != BlackjackPhase::PlayerTurn) {
        return TerminalContextAction::Bet25;
    }
    return BlackjackTable::handValue(cardTable_.playerHand()) < 17
        ? TerminalContextAction::Hit
        : TerminalContextAction::Stand;
}

std::string_view GameTerminal::contextActionLabel(
    TerminalContextAction action
) noexcept
{
    switch (action) {
        case TerminalContextAction::PowerOn: return "POWER ON";
        case TerminalContextAction::OpenCards: return "OPEN CARDS";
        case TerminalContextAction::Bet25: return "BET 25";
        case TerminalContextAction::Hit: return "HIT";
        case TerminalContextAction::Stand: return "STAND";
    }
    return "INTERACT";
}

} // namespace hakui::games
