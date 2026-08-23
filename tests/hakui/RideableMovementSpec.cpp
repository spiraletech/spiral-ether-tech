#include <array>
#include <cassert>
#include <cmath>

#include "player/RideableMovementController.hpp"

namespace {

constexpr std::array<hakui::WorldAffordanceVolume, 3> kAffordances{{
    {1, "TEST MANUAL",
     hakui::affordanceMask(hakui::WorldAffordance::ManualZone),
     -4.0f, 4.0f, -0.5f, 2.0f, -4.0f, 4.0f},
    {2, "TEST RAIL",
     hakui::affordanceMask(hakui::WorldAffordance::Grindable),
     -0.4f, 0.4f, -0.5f, 2.0f, -4.0f, 4.0f,
     {0.0f, 0.45f, 0.0f, 0.0f}},
    {3, "TEST LAUNCH",
     hakui::WorldAffordance::Transition | hakui::WorldAffordance::Launch,
     -4.0f, 4.0f, -0.5f, 2.0f, -4.0f, 4.0f}
}};

hakui::MovementEnvironment testEnvironment()
{
    hakui::MovementEnvironment environment;
    environment.floorMinimumX = -100.0f;
    environment.floorMaximumX = 100.0f;
    environment.floorMinimumZ = -100.0f;
    environment.floorMaximumZ = 100.0f;
    environment.voidResetHeight = -8.0f;
    return environment;
}

void settleLanding(
    hakui::RideableMovementController& controller,
    PlayerState& player,
    hakui::RideableInput input
)
{
    for (int frame = 0; frame < 240 && !player.grounded; ++frame) {
        input.hopPressed = false;
        input.flipLeftPressed = false;
        input.flipRightPressed = false;
        (void)controller.update(
            player,
            input,
            testEnvironment(),
            kAffordances,
            1.0f / 120.0f
        );
    }
}

} // namespace

int main()
{
    using namespace hakui;

    PlayerState skateboard;
    skateboard.locomotion = LocomotionMode::Skateboard;
    RideableMovementController boardController;
    RideableInput boardInput;
    boardInput.movement.forward = 1.0f;
    for (int frame = 0; frame < 90; ++frame) {
        (void)boardController.update(
            skateboard,
            boardInput,
            testEnvironment(),
            kAffordances,
            1.0f / 60.0f
        );
    }
    assert(boardController.state().speed > 3.0f);

    boardInput.hopPressed = true;
    RideableFrame ollie = boardController.update(
        skateboard,
        boardInput,
        testEnvironment(),
        kAffordances,
        1.0f / 60.0f
    );
    assert(ollie.movement.jumped);
    assert(boardController.state().phase == RidePhase::Airborne);
    assert(boardController.state().combo[0] == RideTrick::Ollie);

    boardInput.hopPressed = false;
    boardInput.flipLeftPressed = true;
    RideableFrame flip = boardController.update(
        skateboard,
        boardInput,
        testEnvironment(),
        kAffordances,
        1.0f / 60.0f
    );
    assert(flip.trickStarted);
    assert(boardController.state().activeTrick == RideTrick::Kickflip);
    settleLanding(boardController, skateboard, boardInput);
    assert(skateboard.grounded);
    assert(boardController.state().landingQuality != LandingQuality::None);
    assert(boardController.state().comboCount >= 3);

    boardInput.flipLeftPressed = false;
    boardInput.manualHeld = true;
    skateboard.x = 0.0f;
    skateboard.z = 0.0f;
    RideableFrame manual = boardController.update(
        skateboard,
        boardInput,
        testEnvironment(),
        kAffordances,
        1.0f / 60.0f
    );
    assert(manual.manualStarted);
    assert(boardController.state().phase == RidePhase::Manual);
    assert(boardController.state().activeTrick == RideTrick::BoardManual);

    boardInput.manualHeld = false;
    boardInput.grindHeld = true;
    const RideableFrame groundGrind = boardController.update(
        skateboard,
        boardInput,
        testEnvironment(),
        kAffordances,
        1.0f / 60.0f
    );
    assert(groundGrind.grindStarted);
    assert(boardController.state().phase == RidePhase::Grinding);
    assert(boardController.state().activeTrick == RideTrick::BoardGrind);

    PlayerState bmx;
    bmx.locomotion = LocomotionMode::BMX;
    RideableMovementController bmxController;
    RideableInput bmxInput;
    bmxInput.movement.forward = 1.0f;
    for (int frame = 0; frame < 75; ++frame) {
        (void)bmxController.update(
            bmx,
            bmxInput,
            testEnvironment(),
            kAffordances,
            1.0f / 60.0f
        );
    }
    bmxInput.hopPressed = true;
    const RideableFrame hop = bmxController.update(
        bmx,
        bmxInput,
        testEnvironment(),
        kAffordances,
        1.0f / 60.0f
    );
    assert(hop.movement.jumped);
    assert(bmxController.state().activeTrick == RideTrick::BunnyHop);

    bmxInput.hopPressed = false;
    bmxInput.grindHeld = true;
    bmx.x = 0.0f;
    bmx.z = 0.0f;
    bmx.y = 0.55f;
    bmx.velocityY = -1.0f;
    const RideableFrame peg = bmxController.update(
        bmx,
        bmxInput,
        testEnvironment(),
        kAffordances,
        1.0f / 60.0f
    );
    assert(peg.grindStarted);
    assert(bmxController.state().phase == RidePhase::Grinding);
    assert(bmxController.state().activeTrick == RideTrick::PegGrind);
    assert(std::fabs(bmx.y - 0.45f) < 0.001f);

    bmxController.reset();
    assert(bmxController.state().discipline == RideDiscipline::None);
    assert(RideableMovementController::trickLabel(RideTrick::Kickflip) ==
           "KICKFLIP");
    return 0;
}
