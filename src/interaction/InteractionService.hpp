#pragma once

#include <cstddef>
#include <memory>
#include <unordered_map>
#include <vector>

#include "interaction/Interactable.hpp"
#include "spiral/bus/RouterBus.hpp"

namespace hakui {

// Non-owning interaction registry + execution bridge.
// World/entity ownership remains elsewhere; weak_ptr registration means an
// expired object cannot become a dangling interaction target.
class InteractionService {
public:
    explicit InteractionService(spiral::RouterBus& bus);

    bool registerTarget(const std::shared_ptr<Interactable>& target);
    bool unregisterTarget(EntityId id);
    void pruneExpired();

    std::size_t targetCount();
    std::vector<InteractionOption> options(EntityId actor, EntityId target);
    InteractionResult interact(const InteractionRequest& request);

private:
    std::shared_ptr<Interactable> findTarget(EntityId id);
    bool offersVerb(
        const std::vector<InteractionOption>& options,
        InteractionVerb verb
    ) const;

    void emitRequest(const InteractionRequest& request);
    void emitResult(const InteractionRequest& request, const InteractionResult& result);
    void emitError(const InteractionRequest& request, const char* topic, const char* message);

private:
    spiral::RouterBus& bus_;
    std::unordered_map<EntityId, std::weak_ptr<Interactable>> targets_;
};

} // namespace hakui
