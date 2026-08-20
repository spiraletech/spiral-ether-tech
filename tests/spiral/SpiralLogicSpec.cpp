#include <cassert>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <variant>

#include "spiral/SpiralKernel.hpp"
#include "spiral/crystal/CrystalModule.hpp"
#include "spiral/crystal/ImvuSkeletonCrystal.hpp"
#include "spiral/ledger/MonolithLedger.hpp"
#include "spiral/routing/RouteTable.hpp"

using namespace std::chrono_literals;

namespace {

class SpecCrystal final : public spiral::Crystal {
public:
    static constexpr spiral::CrystalId kId = 0x53504543ULL; // 'SPEC'

    spiral::CrystalId id() const noexcept override { return kId; }
    std::string_view name() const noexcept override { return "crystal.spec_owned"; }

    bool emerge(spiral::RouterBus& bus) override
    {
        (void)bus;
        state_ = State::Sustaining;
        return true;
    }

    void sustain(float dtSeconds) override
    {
        sustainedSeconds_ += dtSeconds;
    }

    void returnToDormant() override
    {
        state_ = State::Dormant;
    }

    void onSignal(const spiral::Signal& signal) override
    {
        lastTopic_ = signal.topic;
    }

    State state() const noexcept override { return state_; }
    bool healthy() const noexcept override { return state_ != State::Faulted; }

    float sustainedSeconds() const noexcept { return sustainedSeconds_; }
    const std::string& lastTopic() const noexcept { return lastTopic_; }

private:
    State state_ = State::Dormant;
    float sustainedSeconds_ = 0.0f;
    std::string lastTopic_;
};

class SpecModule final : public spiral::CrystalModule {
public:
    spiral::Crystal& crystal() noexcept override { return crystal_; }
    const spiral::Crystal& crystal() const noexcept override { return crystal_; }

    SpecCrystal& specCrystal() noexcept { return crystal_; }

private:
    SpecCrystal crystal_;
};

void ether_bus_requires_time_and_disembark()
{
    spiral::EtherBus bus;
    spiral::Signal signal;
    signal.topic = "spec.transition";

    const auto t0 = spiral::Clock::now();

    assert(bus.board(signal, t0));
    assert(bus.occupied());

    bus.tick(t0 + 1999ms);
    assert(!bus.ready());
    assert(!bus.disembark().has_value());

    bus.tick(t0 + spiral::EtherBus::kDefaultTransit);
    assert(bus.ready());

    const auto passenger = bus.disembark();
    assert(passenger.has_value());
    assert(bus.state() == spiral::EtherBus::State::Idle);
}

void octopus_wheels_are_orthogonal_and_bus_gated()
{
    static_assert(!std::is_same_v<spiral::MindWheel, spiral::CodingWheel>);

    using MindAccessor = decltype(std::declval<spiral::SpiralKernel&>().mindWheel());
    using CodingAccessor = decltype(std::declval<spiral::SpiralKernel&>().codingWheel());
    static_assert(std::is_const_v<std::remove_reference_t<MindAccessor>>);
    static_assert(std::is_const_v<std::remove_reference_t<CodingAccessor>>);

    spiral::SpiralKernel kernel;
    const auto t0 = spiral::Clock::now();

    assert(kernel.requestMindPolicy(2, t0));

    // One Ether passenger at a time. Coding remains independent and waits.
    assert(!kernel.requestCodingAction(5, t0));

    kernel.tick(0.1f, t0 + 2499ms);
    assert(!kernel.mindWheel().activeIndex().has_value());

    kernel.tick(0.1f, t0 + 2500ms);
    assert(kernel.etherBus().ready());

    const auto mindTransit = kernel.disembarkTransition();
    assert(mindTransit.has_value());
    assert(kernel.mindWheel().activeIndex() == 2);
    assert(!kernel.codingWheel().activeIndex().has_value());

    const auto t1 = t0 + 3000ms;
    assert(kernel.requestCodingAction(5, t1));
    kernel.tick(0.1f, t1 + 2500ms);
    assert(kernel.disembarkTransition().has_value());

    assert(kernel.mindWheel().activeIndex() == 2);
    assert(kernel.codingWheel().activeIndex() == 5);
}

void route_table_is_ordered_first_match_wins()
{
    spiral::RouteTable routes;
    bool secondConsulted = false;

    assert(routes.addRule(
        "first",
        "crystal.first",
        [](const spiral::Signal&) { return true; }
    ));

    assert(routes.addRule(
        "second",
        "crystal.second",
        [&secondConsulted](const spiral::Signal&) {
            secondConsulted = true;
            return true;
        }
    ));

    spiral::Signal signal;
    signal.topic = "anything";

    const auto match = routes.resolve(signal);
    assert(match.has_value());
    assert(match->index == 0);
    assert(match->name == "first");
    assert(match->destination == "crystal.first");
    assert(!secondConsulted);
}

void kernel_dispatch_uses_route_table_without_inventing_fallback()
{
    spiral::SpiralKernel kernel;
    std::string deliveredTo;

    kernel.router().subscribe([&deliveredTo](const spiral::Signal& signal) {
        if (signal.topic == "route.spec") {
            deliveredTo = signal.destination;
        }
    });

    assert(kernel.routes().addRule(
        "spec",
        "crystal.spec",
        [](const spiral::Signal& signal) {
            return signal.topic == "route.spec";
        }
    ));

    spiral::Signal routed;
    routed.source = "spec";
    routed.topic = "route.spec";
    assert(kernel.dispatch(routed));
    assert(deliveredTo == "crystal.spec");

    spiral::Signal missing;
    missing.source = "spec";
    missing.topic = "no.route";
    assert(!kernel.dispatch(missing));

    const auto tail = kernel.monolith().tail(1);
    assert(tail.size() == 1);
    assert(tail[0].kind == spiral::SignalKind::Error);
    assert(tail[0].topic == "route.miss");
}

void router_bus_sequences_signals_and_monolith_is_bounded()
{
    spiral::RouterBus bus;
    spiral::MonolithLedger ledger(bus, 3);

    spiral::Signal one;
    one.topic = "one";
    const auto id1 = bus.emit(one);

    spiral::Signal two;
    two.topic = "two";
    const auto id2 = bus.emit(two);

    spiral::Signal three;
    three.topic = "three";
    const auto id3 = bus.emit(three);

    spiral::Signal four;
    four.topic = "four";
    const auto id4 = bus.emit(four);

    assert(id1 < id2 && id2 < id3 && id3 < id4);
    assert(ledger.size() == 3);

    const auto snapshot = ledger.snapshot();
    assert(snapshot.size() == 3);
    assert(snapshot[0].topic == "two");
    assert(snapshot[1].topic == "three");
    assert(snapshot[2].topic == "four");
    assert(snapshot[2].id == id4);
}

void state_store_merges_bus_patches_and_restores_snapshots()
{
    spiral::SpiralKernel kernel;

    spiral::Signal first;
    first.kind = spiral::SignalKind::State;
    first.source = "spec";
    first.destination = "spiral.core";
    first.topic = "state.first";
    first.statePatch = {
        {"player.mode", std::string("walk")},
        {"wallet.money", std::int64_t{10}}
    };
    assert(kernel.dispatch(first));

    assert(kernel.stateStore().revision() == 1);
    assert(kernel.stateStore().size() == 2);

    const auto* mode1 = kernel.stateStore().get("player.mode");
    const auto* money1 = kernel.stateStore().get("wallet.money");
    assert(mode1 != nullptr);
    assert(money1 != nullptr);
    assert(std::get<std::string>(*mode1) == "walk");
    assert(std::get<std::int64_t>(*money1) == 10);

    spiral::Signal second;
    second.kind = spiral::SignalKind::State;
    second.source = "spec";
    second.destination = "spiral.core";
    second.topic = "state.second";
    second.statePatch = {
        {"player.mode", std::string("bmx")},
        {"wallet.money", std::int64_t{20}},
        {"wallet.money", std::int64_t{25}}
    };
    assert(kernel.dispatch(second));

    assert(kernel.stateStore().revision() == 2);
    assert(std::get<std::string>(*kernel.stateStore().get("player.mode")) == "bmx");
    assert(std::get<std::int64_t>(*kernel.stateStore().get("wallet.money")) == 25);

    const auto latestEvent = kernel.monolith().tail(1);
    assert(latestEvent.size() == 1);
    assert(kernel.stateStore().lastSignalId() == latestEvent[0].id);

    // Empty-key-only patches are observable events but are not state revisions.
    spiral::Signal ignored;
    ignored.kind = spiral::SignalKind::State;
    ignored.source = "spec";
    ignored.destination = "spiral.core";
    ignored.topic = "state.ignored";
    ignored.statePatch = {
        {"", std::string("ignored")}
    };
    assert(kernel.dispatch(ignored));
    assert(kernel.stateStore().revision() == 2);

    auto snapshot = kernel.stateStore().snapshot();
    kernel.stateStore().clear();
    assert(kernel.stateStore().revision() == 0);
    assert(kernel.stateStore().size() == 0);

    kernel.stateStore().restore(std::move(snapshot));
    assert(kernel.stateStore().revision() == 2);
    assert(kernel.stateStore().size() == 2);
    assert(std::get<std::string>(*kernel.stateStore().get("player.mode")) == "bmx");
    assert(std::get<std::int64_t>(*kernel.stateStore().get("wallet.money")) == 25);
}

void crystal_host_owns_mounts_and_detaches_before_destruction()
{
    spiral::SpiralKernel kernel;

    auto module = std::make_unique<SpecModule>();
    auto* specModule = module.get();
    const spiral::CrystalId id = specModule->crystal().id();

    assert(kernel.crystalHost().mount(std::move(module), 1, 1));
    assert(kernel.crystalHost().size() == 1);
    assert(kernel.crystalGrid().find(id) != nullptr);
    assert(kernel.crystalHost().requestEmerge(id));

    kernel.tick(0.01f);
    assert(specModule->specCrystal().state() == spiral::Crystal::State::Sustaining);

    // U phase should sustain the owned module.
    kernel.tick(3.1f);
    assert(specModule->specCrystal().sustainedSeconds() > 0.0f);

    spiral::Signal signal;
    signal.kind = spiral::SignalKind::Capability;
    signal.source = "spec";
    signal.destination = std::string(specModule->crystal().name());
    signal.topic = "owned.signal";
    assert(kernel.dispatch(signal));
    assert(specModule->specCrystal().lastTopic() == "owned.signal");

    assert(kernel.crystalHost().requestReturn(id));
    kernel.tick(3.0f);
    assert(specModule->specCrystal().state() == spiral::Crystal::State::Dormant);

    assert(kernel.crystalHost().unmount(id));
    assert(kernel.crystalHost().size() == 0);
    assert(kernel.crystalGrid().find(id) == nullptr);
}

void stop_gate_belongs_to_policy()
{
    spiral::SpiralKernel kernel;
    assert(!kernel.mayStop());

    kernel.setMayStopFromPolicy(true);
    assert(kernel.mayStop());

    kernel.setMayStopFromPolicy(false);
    assert(!kernel.mayStop());
}

void steam_engine_contains_overpressure()
{
    spiral::SteamEngine engine;

    engine.charge(150.0f, 10.0f);
    assert(engine.telemetry().safetyValveOpen);
    assert(engine.telemetry().pressure <= 82.0f);

    const std::size_t first = engine.nextPiston();
    const float output = engine.fireNext(10.0f);
    assert(output > 0.0f);
    assert(engine.nextPiston() != first);

    engine.drain();
    assert(engine.telemetry().pressure == 0.0f);
    assert(engine.telemetry().state == spiral::SteamEngine::State::Cooling);
}

void aum_field_cycles_a_u_m()
{
    spiral::AUMField field;

    assert(field.phase() == spiral::AUMPhase::A_Emergence);
    assert(field.coreNode().x == 3);
    assert(field.coreNode().y == 3);

    field.tick(3.1f);
    assert(field.phase() == spiral::AUMPhase::U_Sustain);

    field.tick(3.0f);
    assert(field.phase() == spiral::AUMPhase::M_Return);

    field.tick(3.0f);
    assert(field.phase() == spiral::AUMPhase::A_Emergence);
}

void failed_imvu_crystal_is_contained_and_logged()
{
    spiral::SpiralKernel kernel;
    spiral::ImvuSkeletonCrystal imvu; // no backend hooks = unavailable

    assert(kernel.crystalGrid().attach(imvu, 0, 0));
    assert(kernel.crystalGrid().requestEmerge(imvu.id()));

    // A phase processes emergence. Failure stays inside the crystal grid.
    kernel.tick(0.01f);

    assert(imvu.state() == spiral::Crystal::State::Faulted);
    assert(!imvu.healthy());

    const auto errors = kernel.monolith().tail(1);
    assert(errors.size() == 1);
    assert(errors[0].kind == spiral::SignalKind::Error);
    assert(errors[0].topic == "crystal.error");

    // Kernel plumbing remains alive and usable.
    spiral::Signal signal;
    signal.source = "spec";
    signal.destination = "spiral.core";
    signal.topic = "still.alive";
    assert(kernel.dispatch(signal, 1.0f));
    assert(kernel.steamEngine().telemetry().pressure > 0.0f);
}

} // namespace

int main()
{
    ether_bus_requires_time_and_disembark();
    octopus_wheels_are_orthogonal_and_bus_gated();
    route_table_is_ordered_first_match_wins();
    kernel_dispatch_uses_route_table_without_inventing_fallback();
    router_bus_sequences_signals_and_monolith_is_bounded();
    state_store_merges_bus_patches_and_restores_snapshots();
    crystal_host_owns_mounts_and_detaches_before_destruction();
    stop_gate_belongs_to_policy();
    steam_engine_contains_overpressure();
    aum_field_cycles_a_u_m();
    failed_imvu_crystal_is_contained_and_logged();
    return 0;
}
