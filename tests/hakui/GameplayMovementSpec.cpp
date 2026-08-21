#include <cassert>
#include <cmath>
#include <limits>

#include "player/PlayerMovementController.hpp"
#include "render/Math3D.hpp"
#include "world/BlackRoom.hpp"

namespace {

constexpr float kEpsilon = 0.0001f;

bool nearlyEqual(float left, float right)
{
    return std::fabs(left - right) <= kEpsilon;
}

void diagonal_input_is_normalized()
{
    PlayerState player;
    hakui::PlayerMovementController controller;

    const auto step = controller.update(
        player,
        hakui::MovementInput{1.0f, 1.0f, false},
        0.1f
    );

    assert(step.moved);
    assert(!step.sprinting);
    assert(nearlyEqual(step.distance, 0.325f));
    assert(nearlyEqual(std::sqrt(player.x * player.x + player.z * player.z), 0.325f));
    assert(nearlyEqual(player.yaw, 0.7853982f));
}

void frame_stalls_are_clamped()
{
    PlayerState player;
    hakui::PlayerMovementController controller;

    const auto step = controller.update(
        player,
        hakui::MovementInput{0.0f, 1.0f, false},
        3.0f
    );

    assert(nearlyEqual(step.distance, 0.26f));
    assert(nearlyEqual(player.z, 0.26f));
}

void acceleration_and_deceleration_are_responsive()
{
    PlayerState player;
    hakui::PlayerMovementController controller;

    const auto first = controller.update(
        player,
        hakui::MovementInput{0.0f, 1.0f, false, false},
        0.05f
    );
    const float firstVelocity = player.velocityZ;
    const auto second = controller.update(
        player,
        hakui::MovementInput{0.0f, 1.0f, false, false},
        0.05f
    );

    assert(first.moved);
    assert(second.moved);
    assert(firstVelocity > 0.0f);
    assert(player.velocityZ > firstVelocity);
    assert(second.distance > first.distance);

    controller.update(player, {}, 0.05f);
    assert(player.velocityZ < 2.6f);
    controller.update(player, {}, 0.05f);
    assert(nearlyEqual(player.velocityZ, 0.0f));
}

void sprint_consumes_and_rest_recovers_stamina()
{
    PlayerState player;
    player.stamina = 10.0f;
    hakui::PlayerMovementController controller;

    const auto sprint = controller.update(
        player,
        hakui::MovementInput{0.0f, 1.0f, true},
        0.1f
    );

    assert(sprint.sprinting);
    assert(nearlyEqual(sprint.distance, 0.26f));
    assert(nearlyEqual(player.stamina, 8.8f));

    controller.update(player, {}, 0.1f);
    assert(nearlyEqual(player.stamina, 9.6f));
}

void embodied_modes_use_distinct_motion_and_car_remains_deferred()
{
    hakui::PlayerMovementController controller;

    PlayerState skateboard;
    skateboard.locomotion = LocomotionMode::Skateboard;
    const auto skateStep = controller.update(
        skateboard,
        hakui::MovementInput{0.0f, 1.0f, false, true},
        0.1f
    );
    assert(skateStep.moved);
    assert(!skateStep.jumped);
    assert(skateboard.grounded);

    PlayerState bmx;
    bmx.locomotion = LocomotionMode::BMX;
    const auto bmxStep = controller.update(
        bmx,
        hakui::MovementInput{0.0f, 1.0f, false, true},
        0.1f
    );
    assert(bmxStep.moved);
    assert(!bmxStep.jumped);
    assert(bmx.grounded);
    assert(bmxStep.distance > skateStep.distance);

    PlayerState car;
    car.locomotion = LocomotionMode::Car;
    const auto carStep = controller.update(
        car,
        hakui::MovementInput{1.0f, 1.0f, true, true},
        0.1f
    );

    assert(!carStep.moved);
    assert(nearlyEqual(car.x, 0.0f));
    assert(nearlyEqual(car.z, 0.0f));
}

void invalid_input_cannot_poison_player_state()
{
    PlayerState player;
    hakui::PlayerMovementController controller;
    const float nan = std::numeric_limits<float>::quiet_NaN();

    const auto step = controller.update(
        player,
        hakui::MovementInput{nan, nan, true},
        nan
    );

    assert(!step.moved);
    assert(std::isfinite(player.x));
    assert(std::isfinite(player.z));
    assert(std::isfinite(player.stamina));
}


void jumping_lands_on_room_floor()
{
    PlayerState player;
    hakui::PlayerMovementController controller;
    hakui::MovementEnvironment environment;
    environment.floorMinimumX = -2.0f;
    environment.floorMaximumX = 2.0f;
    environment.floorMinimumZ = -2.0f;
    environment.floorMaximumZ = 2.0f;

    auto step = controller.update(
        player,
        hakui::MovementInput{0.0f, 0.0f, false, true},
        environment,
        0.05f
    );
    assert(step.jumped);
    assert(!player.grounded);
    assert(player.y > 0.0f);

    bool landed = false;
    for (int frame = 0; frame < 100 && !landed; ++frame) {
        step = controller.update(player, {}, environment, 0.05f);
        landed = step.landed;
    }
    assert(landed);
    assert(player.grounded);
    assert(nearlyEqual(player.y, 0.0f));
}

void room_colliders_block_horizontal_motion()
{
    PlayerState player;
    hakui::PlayerMovementController controller;
    const hakui::HorizontalCollider obstacle{0.45f, 1.50f, -1.0f, 1.0f};
    hakui::MovementEnvironment environment;
    environment.colliders = std::span<const hakui::HorizontalCollider>(&obstacle, 1);

    const auto step = controller.update(
        player,
        hakui::MovementInput{1.0f, 0.0f, false, false},
        environment,
        0.1f
    );
    assert(!step.moved);
    assert(nearlyEqual(player.x, 0.0f));
    assert(nearlyEqual(player.velocityX, 0.0f));
}

void black_space_fall_recovers_at_checkpoint()
{
    PlayerState player;
    player.x = 2.0f;
    player.y = -4.95f;
    player.velocityY = -4.0f;
    player.grounded = false;

    hakui::MovementEnvironment environment;
    environment.floorMinimumX = -1.0f;
    environment.floorMaximumX = 1.0f;
    environment.floorMinimumZ = -1.0f;
    environment.floorMaximumZ = 1.0f;
    environment.voidResetHeight = -5.0f;
    environment.spawnX = 0.0f;
    environment.spawnY = 0.0f;
    environment.spawnZ = 0.75f;

    hakui::PlayerMovementController controller;
    const auto step = controller.update(player, {}, environment, 0.1f);
    assert(step.respawned);
    assert(player.grounded);
    assert(nearlyEqual(player.x, 0.0f));
    assert(nearlyEqual(player.y, 0.0f));
    assert(nearlyEqual(player.z, 0.75f));
    assert(player.voidRespawns == 1);
}

void furniture_and_casino_use_contextual_seat_anchors()
{
    hakui::BlackRoom room;
    PlayerState player;
    player.x = 0.0f;
    player.z = 2.05f;

    const auto table = room.nearestInteraction(player);
    assert(table.kind == hakui::RoomInteractionKind::FusionTable);
    assert(room.engageNearest(player));
    assert(player.activity == PlayerActivity::CasinoSeated);
    assert(player.activeAffordanceId == table.affordanceId);
    const auto* tableVolume = room.affordanceById(table.affordanceId);
    assert(tableVolume != nullptr);
    assert(nearlyEqual(player.x, tableVolume->primaryAnchor.x));
    assert(nearlyEqual(player.z, tableVolume->primaryAnchor.z));
    assert(room.leaveInteraction(player));
    assert(player.activity == PlayerActivity::Roaming);
    assert(player.activeAffordanceId == 0);
    assert(nearlyEqual(player.x, tableVolume->secondaryAnchor.x));
    assert(nearlyEqual(player.z, tableVolume->secondaryAnchor.z));

    player.x = 5.72f;
    player.z = 3.28f;
    const auto couch = room.nearestInteraction(player);
    assert(couch.kind == hakui::RoomInteractionKind::LoungeCouch);
    assert(room.engageNearest(player));
    assert(player.activity == PlayerActivity::CouchSeated);
    assert(player.activeAffordanceId == couch.affordanceId);
    const auto* couchVolume = room.affordanceById(couch.affordanceId);
    assert(couchVolume != nullptr);
    assert(nearlyEqual(player.x, couchVolume->primaryAnchor.x));
    assert(nearlyEqual(player.z, couchVolume->primaryAnchor.z));

    const auto seatedStep = hakui::PlayerMovementController{}.update(
        player,
        hakui::MovementInput{1.0f, 1.0f, true, true},
        room.movementEnvironment(),
        0.1f
    );
    assert(!seatedStep.moved);
}

void modular_world_grammar_contains_canonical_roles()
{
    hakui::BlackRoom room;
    const auto geometry = room.geometry();
    assert(geometry.size() >= 30);

    bool hasRamp = false;
    bool hasPlatform = false;
    bool hasMonument = false;
    bool hasFurniture = false;
    bool hasCasino = false;
    bool hasSignage = false;
    bool hasPowderConcrete = false;
    bool hasCrtAccent = false;
    bool hasVoidMaterial = false;
    bool hasRepeatedModule = false;

    for (const hakui::WorldPrimitive& primitive : geometry) {
        hasRamp = hasRamp || primitive.kind == hakui::WorldPrimitiveKind::Ramp;
        hasPlatform = hasPlatform || primitive.kind == hakui::WorldPrimitiveKind::Platform;
        hasMonument = hasMonument || primitive.kind == hakui::WorldPrimitiveKind::Monument;
        hasFurniture = hasFurniture || primitive.kind == hakui::WorldPrimitiveKind::Furniture;
        hasCasino = hasCasino || primitive.kind == hakui::WorldPrimitiveKind::Casino;
        hasSignage = hasSignage || primitive.kind == hakui::WorldPrimitiveKind::Signage;
        hasPowderConcrete = hasPowderConcrete ||
            primitive.material == hakui::MaterialRole::PowderConcrete;
        hasCrtAccent = hasCrtAccent ||
            primitive.material == hakui::MaterialRole::CrtCyan;
        hasVoidMaterial = hasVoidMaterial ||
            primitive.material == hakui::MaterialRole::VoidBlack;
        hasRepeatedModule = hasRepeatedModule || primitive.repeatCount > 1;
    }

    assert(hasRamp && hasPlatform && hasMonument);
    assert(hasFurniture && hasCasino && hasSignage);
    assert(hasPowderConcrete && hasCrtAccent && hasVoidMaterial);
    assert(hasRepeatedModule);
}

void player_can_traverse_ramp_to_elevated_platform()
{
    hakui::BlackRoom room;
    const hakui::MovementEnvironment environment = room.movementEnvironment();
    PlayerState player;
    player.x = -7.8f;
    player.z = 0.0f;
    player.y = environment.groundHeightAt(player.x, player.z).value();
    player.grounded = true;

    hakui::PlayerMovementController controller;
    for (int frame = 0; frame < 30; ++frame) {
        controller.update(
            player,
            hakui::MovementInput{1.0f, 0.0f, false, false},
            environment,
            0.05f
        );
    }

    assert(player.x > -4.0f && player.x < -2.0f);
    assert(player.grounded);
    assert(nearlyEqual(player.y, 2.0f));
}

void world_affordances_describe_system_neutral_actions()
{
    hakui::BlackRoom room;
    assert(room.affordances().size() >= 8);
    assert(room.hasAffordanceAt(
        hakui::WorldAffordance::FightZone,
        5.0f,
        0.0f,
        -5.0f
    ));
    assert(room.hasAffordanceAt(
        hakui::WorldAffordance::SparAnchor,
        5.0f,
        0.0f,
        -5.0f
    ));
    assert(!room.hasAffordanceAt(
        hakui::WorldAffordance::FightZone,
        0.0f,
        0.0f,
        5.25f
    ));
    assert(room.hasAffordanceAt(
        hakui::WorldAffordance::CasinoAnchor,
        0.0f,
        0.0f,
        1.75f
    ));
    assert(room.hasAffordanceAt(
        hakui::WorldAffordance::Void,
        20.0f,
        -3.0f,
        20.0f
    ));

    const auto* fight = room.firstAffordance(hakui::WorldAffordance::FightZone);
    assert(fight != nullptr);
    assert(fight->id != 0);
    assert(!nearlyEqual(fight->primaryAnchor.x, fight->secondaryAnchor.x));
    assert(fight->contains(
        fight->primaryAnchor.x,
        fight->primaryAnchor.y,
        fight->primaryAnchor.z
    ));
    assert(fight->contains(
        fight->secondaryAnchor.x,
        fight->secondaryAnchor.y,
        fight->secondaryAnchor.z
    ));
}

void renderer_rotations_are_stable()
{
    using namespace hakui::math;

    constexpr float halfTurn = 1.57079632679f;
    const Mat4 yaw = rotationY(halfTurn);

    // Column-major transform of a unit forward vector. This guards the matrix
    // convention used by the procedural avatar and camera-relative steering.
    const float transformedX = yaw.m[8];
    const float transformedY = yaw.m[9];
    const float transformedZ = yaw.m[10];
    assert(nearlyEqual(transformedX, 1.0f));
    assert(nearlyEqual(transformedY, 0.0f));
    assert(nearlyEqual(transformedZ, 0.0f));

    const Mat4 combined = multiply(
        translation({2.0f, 3.0f, 4.0f}),
        rotationX(halfTurn)
    );
    assert(nearlyEqual(combined.m[12], 2.0f));
    assert(nearlyEqual(combined.m[13], 3.0f));
    assert(nearlyEqual(combined.m[14], 4.0f));
}

void camera_relative_axes_match_screen_directions()
{
    using namespace hakui::math;

    const Vec3 rightAtZero = cameraRelativePlanarMovement(1.0f, 0.0f, 0.0f);
    assert(nearlyEqual(rightAtZero.x, -1.0f));
    assert(nearlyEqual(rightAtZero.z, 0.0f));

    constexpr float quarterTurn = 1.57079632679f;
    const Vec3 rightAtQuarterTurn = cameraRelativePlanarMovement(
        1.0f,
        0.0f,
        quarterTurn
    );
    assert(nearlyEqual(rightAtQuarterTurn.x, 0.0f));
    assert(nearlyEqual(rightAtQuarterTurn.z, 1.0f));

    const Vec3 forwardAtZero = cameraRelativePlanarMovement(0.0f, 1.0f, 0.0f);
    assert(nearlyEqual(forwardAtZero.x, 0.0f));
    assert(nearlyEqual(forwardAtZero.z, -1.0f));
}

} // namespace

int main()
{
    diagonal_input_is_normalized();
    frame_stalls_are_clamped();
    acceleration_and_deceleration_are_responsive();
    sprint_consumes_and_rest_recovers_stamina();
    jumping_lands_on_room_floor();
    room_colliders_block_horizontal_motion();
    black_space_fall_recovers_at_checkpoint();
    furniture_and_casino_use_contextual_seat_anchors();
    modular_world_grammar_contains_canonical_roles();
    player_can_traverse_ramp_to_elevated_platform();
    world_affordances_describe_system_neutral_actions();
    embodied_modes_use_distinct_motion_and_car_remains_deferred();
    invalid_input_cannot_poison_player_state();
    renderer_rotations_are_stable();
    camera_relative_axes_match_screen_directions();
    return 0;
}
