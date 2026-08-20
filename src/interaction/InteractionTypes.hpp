#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include "spiral/core/SpiralTypes.hpp"

namespace hakui {

using EntityId = std::uint64_t;

enum class InteractionVerb {
    Use,
    Enter,
    Mount,
    PickUp,
    Sit,
    Open,
    Talk,
    Trade,
    Play,
    Inspect
};

constexpr std::string_view interactionVerbName(InteractionVerb verb) noexcept
{
    switch (verb) {
        case InteractionVerb::Use:     return "use";
        case InteractionVerb::Enter:   return "enter";
        case InteractionVerb::Mount:   return "mount";
        case InteractionVerb::PickUp:  return "pick_up";
        case InteractionVerb::Sit:     return "sit";
        case InteractionVerb::Open:    return "open";
        case InteractionVerb::Talk:    return "talk";
        case InteractionVerb::Trade:   return "trade";
        case InteractionVerb::Play:    return "play";
        case InteractionVerb::Inspect: return "inspect";
    }
    return "unknown";
}

struct InteractionOption {
    InteractionVerb verb = InteractionVerb::Use;
    std::string prompt;
};

struct InteractionRequest {
    EntityId actor = 0;
    EntityId target = 0;
    InteractionVerb verb = InteractionVerb::Use;
    std::string argument;
};

struct InteractionResult {
    bool handled = false;
    std::string output;

    // Canonical shared state changes are returned as a Spiral state patch.
    // Interaction objects do not receive a mutable PlayerState reference.
    spiral::StatePatch statePatch;
};

} // namespace hakui
