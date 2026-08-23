#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "games/BlackjackTable.hpp"
#include "games/Dice.hpp"
#include "interaction/Interactable.hpp"

namespace hakui::games {

enum class TerminalModel { NebulaTower, OrchardGlass, FusionDeck };
enum class TerminalApp { Home, CardTable52, DiceLab };
enum class TerminalContextAction { PowerOn, OpenCards, Bet25, Hit, Stand };

constexpr std::string_view terminalModelName(TerminalModel model) noexcept
{
    switch (model) {
        case TerminalModel::NebulaTower: return "Nebula Tower";
        case TerminalModel::OrchardGlass: return "Orchard Glass";
        case TerminalModel::FusionDeck: return "Fusion Deck";
    }
    return "Unknown Terminal";
}

class GameTerminal final : public hakui::Interactable {
public:
    GameTerminal(
        hakui::EntityId id,
        TerminalModel model,
        std::uint32_t seed = 0x48414B55U
    );

    hakui::EntityId interactionId() const noexcept override;
    std::vector<hakui::InteractionOption> interactionOptions(
        hakui::EntityId actor
    ) const override;
    hakui::InteractionResult interact(
        const hakui::InteractionRequest& request
    ) override;

    bool powered() const noexcept;
    TerminalApp activeApp() const noexcept;
    bool beginCardRound(std::int64_t virtualWager);
    bool hitCardTable();
    bool standCardTable();
    const BlackjackTable& cardTable() const noexcept;
    DiceResult lastDiceResult() const;
    std::int64_t virtualCredits() const noexcept;
    std::int64_t lastDiceReward() const noexcept;
    TerminalContextAction nextContextAction() const noexcept;
    static std::string_view contextActionLabel(
        TerminalContextAction action
    ) noexcept;

private:
    hakui::EntityId id_ = 0;
    TerminalModel model_ = TerminalModel::FusionDeck;
    TerminalApp activeApp_ = TerminalApp::Home;
    bool powered_ = false;
    BlackjackTable cardTable_;
    DiceRoller dice_;
    DiceResult lastDice_;
    std::int64_t lastDiceReward_ = 0;
};

} // namespace hakui::games
