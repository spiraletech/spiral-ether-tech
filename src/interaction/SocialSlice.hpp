#pragma once

#include <cstddef>
#include <optional>
#include <span>
#include <string>
#include <utility>
#include <vector>

#include "interaction/Interactable.hpp"

namespace hakui::social {

// A scripted, passive social contact. HAKUI owns when this object exists,
// whether it is in range, and whether the player's input reaches it.
// No dialogue is generated and no shared/canonical state is modified.
class SocialContact final : public Interactable {
public:
    SocialContact(EntityId id, std::string name,
                  std::string greeting, std::string description)
        : id_(id), name_(std::move(name)),
          greeting_(std::move(greeting)), description_(std::move(description)) {}

    EntityId interactionId() const noexcept override { return id_; }

    std::vector<InteractionOption> interactionOptions(EntityId actor) const override {
        if (actor == 0 || id_ == 0 || name_.empty()) return {};
        return {{InteractionVerb::Talk, "Talk to " + name_},
                {InteractionVerb::Inspect, "Inspect " + name_}};
    }

    InteractionResult interact(const InteractionRequest& request) override {
        if (id_ == 0 || request.actor == 0 || request.target != id_ || name_.empty())
            return {};
        if (request.verb == InteractionVerb::Talk)
            return {true, greeting_, {}};
        if (request.verb == InteractionVerb::Inspect)
            return {true, description_, {}};
        return {};
    }

private:
    EntityId id_;
    std::string name_;
    std::string greeting_;
    std::string description_;
};

// The engine supplies trusted receipts *after* InteractionService executes.
// Receipts are observations, never commands, persisted truth or AI-generated
// action decisions. This pure evaluator cannot move the player or an NPC.
struct Receipt {
    InteractionRequest request;
    bool handled = false;
};

struct ObjectiveStep {
    EntityId target = 0;
    InteractionVerb verb = InteractionVerb::Talk;
};

struct Progress {
    std::size_t completed = 0;
    std::size_t total = 0;
    bool complete() const noexcept { return total > 0 && completed == total; }
};

// Nullopt means the objective specification is invalid. Duplicate steps
// are rejected so a repeated receipt cannot accidentally complete two steps.
// Max 16 steps bounds the cost of malformed or untrusted authored content.
inline std::optional<Progress> evaluateObjective(
    EntityId player, std::span<const ObjectiveStep> steps,
    std::span<const Receipt> receipts) noexcept
{
    if (player == 0 || steps.empty() || steps.size() > 16) return std::nullopt;
    for (std::size_t i = 0; i < steps.size(); ++i) {
        const auto& step = steps[i];
        if (step.target == 0 ||
            (step.verb != InteractionVerb::Talk &&
             step.verb != InteractionVerb::Inspect)) return std::nullopt;
        for (std::size_t j = 0; j < i; ++j) {
            if (step.target == steps[j].target && step.verb == steps[j].verb)
                return std::nullopt;
        }
    }

    Progress progress{0, steps.size()};
    for (const auto& receipt : receipts) {
        if (progress.complete()) break;
        const auto& request = receipt.request;
        const auto& next = steps[progress.completed];
        if (receipt.handled && request.actor == player &&
            request.target == next.target && request.verb == next.verb) {
            ++progress.completed;
        }
    }
    return progress;
}

} // namespace hakui::social
