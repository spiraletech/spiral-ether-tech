#include <cassert>
#include <chrono>
#include <type_traits>
#include <utility>

#include "spiral/SpiralKernel.hpp"
#include "spiral/crystal/ImvuSkeletonCrystal.hpp"

using namespace std::chrono_literals;

namespace {

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

void failed_imvu_crystal_is_contained()
{
    spiral::SpiralKernel kernel;
    spiral::ImvuSkeletonCrystal imvu; // no backend hooks = unavailable

    assert(kernel.crystalGrid().attach(imvu, 0, 0));
    assert(kernel.crystalGrid().requestEmerge(imvu.id()));

    // A phase processes emergence. Failure stays inside the crystal grid.
    kernel.tick(0.01f);

    assert(imvu.state() == spiral::Crystal::State::Faulted);
    assert(!imvu.healthy());

    // Kernel plumbing remains alive and usable.
    spiral::Signal signal;
    signal.source = "spec";
    signal.destination = "spiral.core";
    signal.topic = "still.alive";
    kernel.pressureRail().send(signal, 1.0f);
    assert(kernel.steamEngine().telemetry().pressure > 0.0f);
}

} // namespace

int main()
{
    ether_bus_requires_time_and_disembark();
    octopus_wheels_are_orthogonal_and_bus_gated();
    stop_gate_belongs_to_policy();
    steam_engine_contains_overpressure();
    aum_field_cycles_a_u_m();
    failed_imvu_crystal_is_contained();
    return 0;
}
