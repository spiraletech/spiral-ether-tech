#pragma once

#include <cstdint>
#include <string_view>

namespace hakui {

// Materials are semantic roles, not renderer assets. A future editor or high-
// fidelity renderer can reinterpret them without changing the world layout.
enum class MaterialRole : std::uint8_t {
    PowderConcrete,
    IndustrialDark,
    CrtCyan,
    SignalMagenta,
    SodiumAmber,
    VoidBlack,
    TerminalGreen,
    HazardRed
};

enum class WorldPrimitiveKind : std::uint8_t {
    Floor,
    Wall,
    Platform,
    Ramp,
    Furniture,
    Casino,
    Terminal,
    Mobility,
    Signage,
    Monument,
    VoidMarker
};

enum class WorldAffordance : std::uint64_t {
    None = 0,
    Rideable = 1ULL << 0,
    Grindable = 1ULL << 1,
    Transition = 1ULL << 2,
    Launch = 1ULL << 3,
    Landing = 1ULL << 4,
    ManualZone = 1ULL << 5,
    StallAnchor = 1ULL << 6,
    Seat = 1ULL << 7,
    CasinoAnchor = 1ULL << 8,
    Terminal = 1ULL << 9,
    FightZone = 1ULL << 10,
    SparAnchor = 1ULL << 11,
    SpectatorZone = 1ULL << 12,
    RespawnVolume = 1ULL << 13,
    Void = 1ULL << 14,
    DuelZone = 1ULL << 15,
    ArcheryLane = 1ULL << 16,
    WeaponRack = 1ULL << 17,
    Target = 1ULL << 18
};

using WorldAffordanceMask = std::uint64_t;

constexpr WorldAffordanceMask affordanceMask(WorldAffordance affordance) noexcept
{
    return static_cast<WorldAffordanceMask>(affordance);
}

constexpr WorldAffordanceMask operator|(
    WorldAffordance left,
    WorldAffordance right
) noexcept
{
    return affordanceMask(left) | affordanceMask(right);
}

constexpr WorldAffordanceMask operator|(
    WorldAffordanceMask left,
    WorldAffordance right
) noexcept
{
    return left | affordanceMask(right);
}

constexpr WorldAffordanceMask operator|(
    WorldAffordance left,
    WorldAffordanceMask right
) noexcept
{
    return affordanceMask(left) | right;
}

constexpr bool hasAffordance(
    WorldAffordanceMask mask,
    WorldAffordance affordance
) noexcept
{
    return (mask & affordanceMask(affordance)) != 0;
}

struct WorldPrimitive {
    WorldPrimitiveKind kind = WorldPrimitiveKind::Platform;
    MaterialRole material = MaterialRole::PowderConcrete;
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float width = 1.0f;
    float height = 1.0f;
    float depth = 1.0f;
    float rotationX = 0.0f;
    float rotationY = 0.0f;
    float rotationZ = 0.0f;
    std::uint16_t repeatCount = 1;
    float repeatX = 0.0f;
    float repeatY = 0.0f;
    float repeatZ = 0.0f;
};

struct WorldAnchor {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float yaw = 0.0f;
};

// Volumes and anchors are gameplay notation. They advertise possible actions;
// the world layer never implements skate, vehicle, combat, casino, or AI rules.
struct WorldAffordanceVolume {
    std::uint32_t id = 0;
    std::string_view label{};
    WorldAffordanceMask affordances = 0;
    float minimumX = 0.0f;
    float maximumX = 0.0f;
    float minimumY = 0.0f;
    float maximumY = 0.0f;
    float minimumZ = 0.0f;
    float maximumZ = 0.0f;
    WorldAnchor primaryAnchor{};
    WorldAnchor secondaryAnchor{};

    bool contains(float x, float y, float z) const noexcept
    {
        return x >= minimumX && x <= maximumX &&
               y >= minimumY && y <= maximumY &&
               z >= minimumZ && z <= maximumZ;
    }
};

} // namespace hakui
