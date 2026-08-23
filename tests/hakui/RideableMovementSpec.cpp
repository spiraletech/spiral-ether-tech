#include <array>
#include <cassert>
#include <cmath>
#include <utility>

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
        input.popPressed = false;
        input.trick = {};
        input.stylePressed = false;
        (void)controller.update(
            player,
            input,
            testEnvironment(),
            kAffordances,
            1.0f / 120.0f
        );
    }
}

std::pair<hakui::RideTrick, hakui::RideableState> startAirTrick(
    LocomotionMode locomotion,
    hakui::RideTrickDirection direction
)
{
    PlayerState player;
    player.locomotion = locomotion;
    player.grounded = false;
    player.y = 3.0f;
    player.velocityY = 1.0f;
    hakui::RideableMovementController controller;
    hakui::RideableInput input;
    input.trick = {direction, 1.0f, 0.0f, 1.0f, 0.08f, true};
    const hakui::RideableFrame frame = controller.update(
        player,
        input,
        testEnvironment(),
        kAffordances,
        1.0f / 60.0f
    );
    assert(frame.trickStarted);
    return {controller.state().activeTrick, controller.state()};
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
    assert(std::fabs(boardController.state().body.pelvisYawRelativeToBoard) > 1.2f);
    assert(std::fabs(boardController.state().body.torsoYawRelativeToBoard) <
           std::fabs(boardController.state().body.pelvisYawRelativeToBoard));

    // Preload is a body state before it becomes a pop impulse.
    PlayerState preloadPlayer;
    preloadPlayer.locomotion = LocomotionMode::Skateboard;
    RideableMovementController preloadController;
    RideableInput preloadInput;
    preloadInput.popPreload = 0.80f;
    (void)preloadController.update(
        preloadPlayer, preloadInput, testEnvironment(), kAffordances,
        1.0f / 60.0f
    );
    assert(preloadController.state().body.preloadPoseWeight > 0.79f);
    assert(preloadController.state().body.rightKneeFlex > 1.0f);
    preloadController.setSkateStance(SkateStance::Goofy);
    (void)preloadController.update(
        preloadPlayer, preloadInput, testEnvironment(), kAffordances,
        1.0f / 60.0f
    );
    assert(preloadController.state().body.skateStance == SkateStance::Goofy);
    assert(preloadController.state().body.pelvisYawRelativeToBoard < -1.2f);

    // Pop preload affects the deterministic impulse only inside a safe cap.
    PlayerState normalPopPlayer;
    normalPopPlayer.locomotion = LocomotionMode::Skateboard;
    normalPopPlayer.x = 20.0f;
    normalPopPlayer.z = 20.0f;
    RideableMovementController normalPopController;
    RideableInput normalPopInput;
    normalPopInput.popPressed = true;
    normalPopInput.popPreload = 0.0f;
    (void)normalPopController.update(
        normalPopPlayer, normalPopInput, testEnvironment(), kAffordances,
        1.0f / 60.0f
    );
    const float normalImpulse = normalPopController.state().popImpulse;
    PlayerState chargedPopPlayer;
    chargedPopPlayer.locomotion = LocomotionMode::Skateboard;
    chargedPopPlayer.x = 20.0f;
    chargedPopPlayer.z = 20.0f;
    RideableMovementController chargedPopController;
    RideableInput chargedPopInput;
    chargedPopInput.popPressed = true;
    chargedPopInput.popPreload = 50.0f;
    (void)chargedPopController.update(
        chargedPopPlayer, chargedPopInput, testEnvironment(), kAffordances,
        1.0f / 60.0f
    );
    assert(chargedPopController.state().popImpulse > normalImpulse);
    assert(std::fabs(
        chargedPopController.state().popImpulse -
        chargedPopController.tuning().skateboardPopImpulseMax
    ) < 0.001f);

    boardInput.popPressed = true;
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
    assert(boardController.state().body.airPose == RideAirPose::OlliePop);
    assert(boardController.state().body.footContact ==
           RideFootContactState::ReleasedForTrick);

    boardInput.popPressed = false;
    boardInput.trick = {RideTrickDirection::Left, -1.0f, 0.0f, 1.0f, 0.12f, true};
    RideableFrame flip = boardController.update(
        skateboard,
        boardInput,
        testEnvironment(),
        kAffordances,
        1.0f / 60.0f
    );
    assert(flip.trickStarted);
    assert(boardController.state().activeTrick == RideTrick::Kickflip);
    assert(boardController.state().body.airPose == RideAirPose::Kickflip);
    assert(boardController.state().body.leftKneeFlex !=
           boardController.state().body.rightKneeFlex);
    settleLanding(boardController, skateboard, boardInput);
    assert(skateboard.grounded);
    assert(boardController.state().phase != RidePhase::Crash);
    assert(boardController.state().landingQuality == LandingQuality::Clean ||
           boardController.state().landingQuality == LandingQuality::Sketchy);
    assert(boardController.state().rotationCompletion >= 0.80f);
    assert(boardController.state().comboCount >= 3);
    boardInput.trick = {};
    (void)boardController.update(
        skateboard, boardInput, testEnvironment(), kAffordances, 0.10f
    );
    assert(boardController.state().body.footContact ==
           RideFootContactState::Landed);
    assert(boardController.state().body.landingCompression > 0.0f);

    // Every v0.84 minimum trick maps to a distinct physical channel/axis
    // profile rather than a renderer-authored canned animation.
    assert(startAirTrick(LocomotionMode::Skateboard, RideTrickDirection::Left).first ==
           RideTrick::Kickflip);
    assert(startAirTrick(LocomotionMode::Skateboard, RideTrickDirection::Right).first ==
           RideTrick::Heelflip);
    assert(startAirTrick(LocomotionMode::Skateboard, RideTrickDirection::Up).first ==
           RideTrick::PopShoveIt);
    assert(startAirTrick(LocomotionMode::Skateboard, RideTrickDirection::Down).first ==
           RideTrick::Impossible);
    assert(startAirTrick(LocomotionMode::Skateboard, RideTrickDirection::UpRight).first ==
           RideTrick::VarialFlip);
    assert(startAirTrick(LocomotionMode::BMX, RideTrickDirection::Left).first ==
           RideTrick::BmxTailwhipLeft);
    assert(startAirTrick(LocomotionMode::BMX, RideTrickDirection::Up).first ==
           RideTrick::BmxBarspin);
    assert(startAirTrick(LocomotionMode::BMX, RideTrickDirection::Down).first ==
           RideTrick::BmxCrankflip);
    assert(startAirTrick(LocomotionMode::BMX, RideTrickDirection::UpLeft).first ==
           RideTrick::BmxXUp);
    assert(startAirTrick(LocomotionMode::BMX, RideTrickDirection::DownRight).first ==
           RideTrick::BmxTabletop);
    assert(RideableMovementController::physicalIntentFor(RideTrick::Impossible)
               .rotationTarget > 6.0f);
    assert(RideableMovementController::physicalIntentFor(RideTrick::BmxBarspin)
               .channel == RideRotationChannel::BmxSteering);

    // A low, unfinished kickflip reaches ground with retained rotation and
    // deterministically bails instead of snapping flat.
    PlayerState underRotated;
    underRotated.locomotion = LocomotionMode::Skateboard;
    underRotated.grounded = false;
    underRotated.y = 0.24f;
    underRotated.velocityY = -1.5f;
    RideableMovementController underRotatedController;
    RideableInput underRotatedInput;
    underRotatedInput.trick = {
        RideTrickDirection::Left, -1.0f, 0.0f, 1.0f, 0.05f, true
    };
    (void)underRotatedController.update(
        underRotated, underRotatedInput, testEnvironment(), kAffordances,
        1.0f / 60.0f
    );
    underRotatedInput.trick = {};
    RideableFrame failedLanding;
    for (int frameIndex = 0; frameIndex < 60 && !underRotated.grounded;
         ++frameIndex) {
        failedLanding = underRotatedController.update(
            underRotated, underRotatedInput, testEnvironment(), kAffordances,
            1.0f / 60.0f
        );
    }
    assert(failedLanding.bailed);
    assert(underRotatedController.state().phase == RidePhase::Crash);
    assert(underRotatedController.state().landingQuality == LandingQuality::Failed);
    assert(underRotatedController.state().bailReason == BailReason::UnderRotated ||
           underRotatedController.state().bailReason ==
               BailReason::ExcessiveAngularVelocity);
    assert(std::fabs(underRotatedController.state().rideableRotation.z) > 0.01f);
    assert(underRotatedController.state().body.airPose == RideAirPose::Bail);
    assert(underRotatedController.state().body.footContact ==
           RideFootContactState::ReleasedForTrick);

    boardInput.trick = {};
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
    assert(boardController.state().activeGrindAttachment ==
           RideGrindAttachment::BoardTrucks);

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
    bmxInput.popPressed = true;
    const RideableFrame hop = bmxController.update(
        bmx,
        bmxInput,
        testEnvironment(),
        kAffordances,
        1.0f / 60.0f
    );
    assert(hop.movement.jumped);
    assert(bmxController.state().activeTrick == RideTrick::BunnyHop);
    assert(bmxController.state().body.airPose == RideAirPose::BmxPull);
    assert(bmxController.state().body.leftElbowFlex > 0.0f);

    bmxInput.popPressed = false;
    bmxInput.trick = {RideTrickDirection::Up, 0.0f, -1.0f, 1.0f, 0.12f, true};
    const RideableFrame barspin = bmxController.update(
        bmx,
        bmxInput,
        testEnvironment(),
        kAffordances,
        1.0f / 60.0f
    );
    assert(barspin.trickStarted);
    assert(bmxController.state().activeTrick == RideTrick::BmxBarspin);
    settleLanding(bmxController, bmx, bmxInput);
    assert(bmx.grounded);
    assert(bmxController.state().phase != RidePhase::Crash);
    assert(bmxController.state().landingQuality == LandingQuality::Clean ||
           bmxController.state().landingQuality == LandingQuality::Sketchy);
    assert(bmxController.state().rotationCompletion >= 0.80f);
    bmxInput.trick = {};
    bmxInput.grindHeld = true;
    bmx.x = 0.0f;
    bmx.z = 0.0f;
    bmx.y = 0.0f;
    bmx.velocityY = 0.0f;
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
    assert(bmxController.state().activeGrindAttachment ==
           RideGrindAttachment::BmxPegs);
    assert(std::fabs(bmx.y - 0.45f) < 0.001f);

    PlayerState perpendicular;
    perpendicular.locomotion = LocomotionMode::Skateboard;
    perpendicular.velocityX = 5.0f;
    perpendicular.velocityZ = 0.0f;
    perpendicular.x = 0.0f;
    perpendicular.z = 0.0f;
    RideableMovementController perpendicularController;
    RideableInput perpendicularInput;
    perpendicularInput.grindHeld = true;
    const RideableFrame rejectedGrind = perpendicularController.update(
        perpendicular,
        perpendicularInput,
        testEnvironment(),
        kAffordances,
        1.0f / 60.0f
    );
    assert(!rejectedGrind.grindStarted);

    boardInput.spinRight = true;
    skateboard.grounded = false;
    skateboard.y = 2.0f;
    const float previousYaw = skateboard.yaw;
    (void)boardController.update(
        skateboard,
        boardInput,
        testEnvironment(),
        kAffordances,
        1.0f / 30.0f
    );
    assert(boardController.state().spinVelocity > 0.0f);
    assert(skateboard.yaw > previousYaw);

    bmxController.reset();
    assert(bmxController.state().discipline == RideDiscipline::None);
    assert(RideableMovementController::trickLabel(RideTrick::Kickflip) ==
           "KICKFLIP");
    return 0;
}
