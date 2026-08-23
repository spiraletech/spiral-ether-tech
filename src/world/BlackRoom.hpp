#pragma once

#include <array>
#include <cstdint>
#include <span>
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

enum class SeatPoseProfile : std::uint8_t {
    CasinoUpright,
    LoungeRelaxed
};

// Local-space furniture semantics. A seat slot belongs to an affordance, can
// be reserved independently, and resolves through that furniture's anchor.
// This is deliberately usable by future chairs/benches/social furniture.
struct SeatAnchor {
    std::uint32_t id = 0;
    std::uint32_t furnitureAffordanceId = 0;
    std::string_view label{};
    WorldAnchor localPosition{};
    bool occupied = false;
    SeatPoseProfile poseProfile = SeatPoseProfile::LoungeRelaxed;
};

struct ResolvedSeatAnchor {
    std::uint32_t id = 0;
    std::uint32_t furnitureAffordanceId = 0;
    WorldAnchor worldPosition{};
    bool occupied = false;
    SeatPoseProfile poseProfile = SeatPoseProfile::LoungeRelaxed;
};

struct RoomInteractionFocus {
    RoomInteractionKind kind = RoomInteractionKind::None;
    std::uint32_t affordanceId = 0;
    std::uint32_t seatAnchorId = 0;
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
    const WorldAffordanceVolume* firstAffordance(
        WorldAffordance affordance
    ) const noexcept;
    const WorldAffordanceVolume* affordanceAt(
        WorldAffordance affordance,
        float x,
        float y,
        float z
    ) const noexcept;
    const WorldAffordanceVolume* affordanceById(
        std::uint32_t id
    ) const noexcept;
    bool hasAffordanceAt(
        WorldAffordance affordance,
        float x,
        float y,
        float z
    ) const noexcept;
    std::span<const SeatAnchor> seatAnchors() const noexcept;
    const SeatAnchor* seatAnchorById(std::uint32_t id) const noexcept;
    ResolvedSeatAnchor resolvedSeatAnchor(std::uint32_t id) const noexcept;
    bool seatOccupied(std::uint32_t id) const noexcept;
    float seatAlignmentError(const PlayerState& player) const noexcept;
    RoomInteractionFocus nearestInteraction(const PlayerState& player) const noexcept;
    bool engageNearest(PlayerState& player) noexcept;
    bool leaveInteraction(PlayerState& player) noexcept;

private:
    SeatAnchor* mutableSeatAnchorById(std::uint32_t id) noexcept;
    const SeatAnchor* nearestAvailableSeat(
        std::uint32_t furnitureAffordanceId,
        const PlayerState& player
    ) const noexcept;

    static const std::array<HorizontalCollider, 5> colliders_;
    static const std::array<WalkableSurface, 3> surfaces_;
    std::array<SeatAnchor, 3> seatAnchors_{{
        {100101, 1001, "FUSION TABLE SLOT",
         {0.0f, 0.0f, 0.0f, 0.0f}, false,
         SeatPoseProfile::CasinoUpright},
        {100201, 1002, "VOID COUCH LEFT",
         {-0.82f, 0.0f, 0.0f, 0.0f}, false,
         SeatPoseProfile::LoungeRelaxed},
        {100202, 1002, "VOID COUCH RIGHT",
         {0.82f, 0.0f, 0.0f, 0.0f}, false,
         SeatPoseProfile::LoungeRelaxed}
    }};
};

} // namespace hakui
