#include "interaction/InteractionService.hpp"

#include <string>
#include <utility>

namespace hakui {

namespace {

std::string interactionPayload(const InteractionRequest& request)
{
    return std::string(interactionVerbName(request.verb)) +
           "|actor=" + std::to_string(request.actor) +
           "|target=" + std::to_string(request.target);
}

} // namespace

InteractionService::InteractionService(spiral::RouterBus& bus)
    : bus_(bus)
{
}

bool InteractionService::registerTarget(const std::shared_ptr<Interactable>& target)
{
    if (!target || target->interactionId() == 0) {
        return false;
    }

    const EntityId id = target->interactionId();
    const auto it = targets_.find(id);
    if (it != targets_.end()) {
        if (!it->second.expired()) {
            return false;
        }
        targets_.erase(it);
    }

    targets_[id] = target;
    return true;
}

bool InteractionService::unregisterTarget(EntityId id)
{
    return targets_.erase(id) > 0;
}

void InteractionService::pruneExpired()
{
    for (auto it = targets_.begin(); it != targets_.end();) {
        if (it->second.expired()) {
            it = targets_.erase(it);
        } else {
            ++it;
        }
    }
}

std::size_t InteractionService::targetCount()
{
    pruneExpired();
    return targets_.size();
}

std::vector<InteractionOption> InteractionService::options(EntityId actor, EntityId target)
{
    const auto object = findTarget(target);
    if (!object) {
        return {};
    }

    return object->interactionOptions(actor);
}

InteractionResult InteractionService::interact(const InteractionRequest& request)
{
    emitRequest(request);

    if (request.actor == 0) {
        emitError(request, "interaction.invalid_actor", "actor id must be nonzero");
        return {};
    }

    const auto object = findTarget(request.target);
    if (!object) {
        emitError(request, "interaction.target_missing", "target is not registered");
        return {};
    }

    const auto offered = object->interactionOptions(request.actor);
    if (!offersVerb(offered, request.verb)) {
        emitError(request, "interaction.denied", "requested verb is not currently offered");
        return {};
    }

    InteractionResult result = object->interact(request);
    if (!result.handled) {
        emitError(request, "interaction.unhandled", "target declined interaction");
        return result;
    }

    emitResult(request, result);
    return result;
}

std::shared_ptr<Interactable> InteractionService::findTarget(EntityId id)
{
    if (id == 0) {
        return {};
    }

    const auto it = targets_.find(id);
    if (it == targets_.end()) {
        return {};
    }

    auto target = it->second.lock();
    if (!target) {
        targets_.erase(it);
    }
    return target;
}

bool InteractionService::offersVerb(
    const std::vector<InteractionOption>& options,
    InteractionVerb verb
) const
{
    for (const InteractionOption& option : options) {
        if (option.verb == verb) {
            return true;
        }
    }
    return false;
}

void InteractionService::emitRequest(const InteractionRequest& request)
{
    spiral::Signal signal;
    signal.kind = spiral::SignalKind::CommandIn;
    signal.source = "hakui.interaction";
    signal.destination = "spiral.core";
    signal.topic = "interaction.request";
    signal.payload = interactionPayload(request);
    bus_.emit(std::move(signal));
}

void InteractionService::emitResult(
    const InteractionRequest& request,
    const InteractionResult& result
)
{
    spiral::Signal executed;
    executed.kind = spiral::SignalKind::Exec;
    executed.source = "hakui.interaction";
    executed.destination = "spiral.core";
    executed.topic = "interaction.exec";
    executed.payload = interactionPayload(request);
    if (!result.output.empty()) {
        executed.payload += "|output=" + result.output;
    }
    bus_.emit(std::move(executed));

    if (!result.statePatch.empty()) {
        spiral::Signal state;
        state.kind = spiral::SignalKind::State;
        state.source = "hakui.interaction";
        state.destination = "spiral.core";
        state.topic = "interaction.state";
        state.payload = interactionPayload(request);
        state.statePatch = result.statePatch;
        bus_.emit(std::move(state));
    }
}

void InteractionService::emitError(
    const InteractionRequest& request,
    const char* topic,
    const char* message
)
{
    spiral::Signal error;
    error.kind = spiral::SignalKind::Error;
    error.source = "hakui.interaction";
    error.destination = "spiral.core";
    error.topic = topic;
    error.payload = interactionPayload(request) + "|error=" + message;
    bus_.emit(std::move(error));
}

} // namespace hakui
