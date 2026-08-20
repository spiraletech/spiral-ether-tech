#pragma once

#include <vector>

#include "interaction/InteractionTypes.hpp"

namespace hakui {

// Capability surface for an in-world object.
//
// The object may update its own private/local runtime state while handling an
// interaction, but shared Hakui state must be returned through InteractionResult
// so it can travel over Router Bus into StateStore/Monolith.
class Interactable {
public:
    virtual ~Interactable() = default;

    virtual EntityId interactionId() const noexcept = 0;
    virtual std::vector<InteractionOption> interactionOptions(EntityId actor) const = 0;
    virtual InteractionResult interact(const InteractionRequest& request) = 0;
};

} // namespace hakui
