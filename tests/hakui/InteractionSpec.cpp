#include <cassert>
#include <cstdint>
#include <memory>
#include <string>
#include <variant>

#include "interaction/InteractionService.hpp"
#include "spiral/SpiralKernel.hpp"

namespace {

class DoorInteractable final : public hakui::Interactable {
public:
    explicit DoorInteractable(hakui::EntityId id)
        : id_(id) {}

    hakui::EntityId interactionId() const noexcept override
    {
        return id_;
    }

    std::vector<hakui::InteractionOption> interactionOptions(
        hakui::EntityId actor
    ) const override
    {
        (void)actor;
        return {
            {hakui::InteractionVerb::Open, open_ ? "Close door" : "Open door"},
            {hakui::InteractionVerb::Inspect, "Inspect door"}
        };
    }

    hakui::InteractionResult interact(
        const hakui::InteractionRequest& request
    ) override
    {
        hakui::InteractionResult result;

        if (request.verb == hakui::InteractionVerb::Open) {
            open_ = !open_;
            result.handled = true;
            result.output = open_ ? "opened" : "closed";
            result.statePatch = {
                {"world.door.100.open", open_}
            };
            return result;
        }

        if (request.verb == hakui::InteractionVerb::Inspect) {
            result.handled = true;
            result.output = open_ ? "door is open" : "door is closed";
            return result;
        }

        return result;
    }

private:
    hakui::EntityId id_ = 0;
    bool open_ = false;
};

void interaction_flows_request_exec_state_into_spiral()
{
    spiral::SpiralKernel kernel;
    hakui::InteractionService interactions(kernel.router());

    auto door = std::make_shared<DoorInteractable>(100);
    assert(interactions.registerTarget(door));
    assert(interactions.targetCount() == 1);

    const auto options = interactions.options(1, 100);
    assert(options.size() == 2);
    assert(options[0].verb == hakui::InteractionVerb::Open);

    hakui::InteractionRequest request;
    request.actor = 1;
    request.target = 100;
    request.verb = hakui::InteractionVerb::Open;

    const auto result = interactions.interact(request);
    assert(result.handled);
    assert(result.output == "opened");

    // Request + exec + state should all be visible to Monolith.
    const auto tail = kernel.monolith().tail(3);
    assert(tail.size() == 3);
    assert(tail[0].topic == "interaction.request");
    assert(tail[1].topic == "interaction.exec");
    assert(tail[2].topic == "interaction.state");

    // Shared world state is reduced only from the returned State patch.
    assert(kernel.stateStore().revision() == 1);
    const auto* open = kernel.stateStore().get("world.door.100.open");
    assert(open != nullptr);
    assert(std::get<bool>(*open));
}

void unavailable_or_missing_interactions_are_observable_errors()
{
    spiral::SpiralKernel kernel;
    hakui::InteractionService interactions(kernel.router());

    auto door = std::make_shared<DoorInteractable>(100);
    assert(interactions.registerTarget(door));

    hakui::InteractionRequest denied;
    denied.actor = 1;
    denied.target = 100;
    denied.verb = hakui::InteractionVerb::Enter;

    const auto deniedResult = interactions.interact(denied);
    assert(!deniedResult.handled);

    auto tail = kernel.monolith().tail(1);
    assert(tail.size() == 1);
    assert(tail[0].kind == spiral::SignalKind::Error);
    assert(tail[0].topic == "interaction.denied");

    hakui::InteractionRequest missing;
    missing.actor = 1;
    missing.target = 999;
    missing.verb = hakui::InteractionVerb::Use;

    const auto missingResult = interactions.interact(missing);
    assert(!missingResult.handled);

    tail = kernel.monolith().tail(1);
    assert(tail.size() == 1);
    assert(tail[0].kind == spiral::SignalKind::Error);
    assert(tail[0].topic == "interaction.target_missing");
}

void expired_world_objects_cannot_become_dangling_targets()
{
    spiral::SpiralKernel kernel;
    hakui::InteractionService interactions(kernel.router());

    auto door = std::make_shared<DoorInteractable>(100);
    assert(interactions.registerTarget(door));
    assert(interactions.targetCount() == 1);

    door.reset();

    // Service owns only weak references and prunes dead world objects.
    assert(interactions.targetCount() == 0);
    assert(interactions.options(1, 100).empty());
}

} // namespace

int main()
{
    interaction_flows_request_exec_state_into_spiral();
    unavailable_or_missing_interactions_are_observable_errors();
    expired_world_objects_cannot_become_dangling_targets();
    return 0;
}
