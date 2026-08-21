#pragma once

#include <array>
#include <string_view>

#include "player/PlayerMovementController.hpp"
#include "player/PlayerState.hpp"
#include "world/WorldGeometry.hpp"

namespace hakui {

enum class RoomInteractionKind {
    None,
    FusionTable,
    LoungeCouch
};

struct RoomInteractionFocus {
    RoomInteractionKind kind = RoomInteractionKind::None;
    float distance = 0.0f;
    std::string_view prompt{};

    explicit operator bool() const noexcept
    {
        return kind != RoomInteractionKind::None;
    }
};

class BlackRoom {
public:
    static constexpr float floorMinimumX = -10.0f;
    static constexpr float floorMaximumX = 10.0f;
    static constexpr float floorMinimumZ = -8.0f;
    static constexpr float floorMaximumZ = 8.0f;

    MovementEnvironment movementEnvironment() const noexcept;
    std::span<const WorldPrimitive> geometry() const noexcept;
    std::span<const WorldAffordanceVolume> affordances() const noexcept;
    bool hasAffordanceAt(
        WorldAffordance affordance,
        float x,
        float y,
        float z
    ) const noexcept;
    RoomInteractionFocus nearestInteraction(const PlayerState& player) const noexcept;
    bool engageNearest(PlayerState& player) const noexcept;
    bool leaveInteraction(PlayerState& player) const noexcept;

private:
    static const std::array<HorizontalCollider, 5> colliders_;
    static const std::array<WalkableSurface, 2> surfaces_;
};

} // namespace hakui
