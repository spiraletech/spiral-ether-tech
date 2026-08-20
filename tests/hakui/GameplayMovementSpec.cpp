#include <cassert>
#include <cmath>
#include <limits>

#include "player/PlayerMovementController.hpp"

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

    assert(nearlyEqual(step.distance, 0.325f));
    assert(nearlyEqual(player.z, 0.325f));
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
    assert(nearlyEqual(sprint.distance, 0.575f));
    assert(nearlyEqual(player.stamina, 8.8f));

    controller.update(player, {}, 0.1f);
    assert(nearlyEqual(player.stamina, 9.6f));
}

void non_on_foot_modes_do_not_apply_on_foot_motion()
{
    PlayerState player;
    player.locomotion = LocomotionMode::BMX;
    player.stamina = 50.0f;
    hakui::PlayerMovementController controller;

    const auto step = controller.update(
        player,
        hakui::MovementInput{1.0f, 1.0f, true},
        0.1f
    );

    assert(!step.moved);
    assert(nearlyEqual(player.x, 0.0f));
    assert(nearlyEqual(player.z, 0.0f));
    assert(player.stamina > 50.0f);
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

} // namespace

int main()
{
    diagonal_input_is_normalized();
    frame_stalls_are_clamped();
    sprint_consumes_and_rest_recovers_stamina();
    non_on_foot_modes_do_not_apply_on_foot_motion();
    invalid_input_cannot_poison_player_state();
    return 0;
}
