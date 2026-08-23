#include "world/BlackRoom.hpp"

#include <cmath>
#include <limits>
#include <span>

namespace hakui {

namespace {

constexpr float kRampAngle = 0.463647609f;
constexpr float kKickerAngle = 0.363147009f;

const auto kSpecimenGeometry = std::to_array<WorldPrimitive>({
    // Plaza and modular CRT datum grid.
    {WorldPrimitiveKind::Floor, MaterialRole::IndustrialDark,
     0.0f, -0.14f, 0.0f, 20.0f, 0.28f, 16.0f},
    {WorldPrimitiveKind::Floor, MaterialRole::VoidBlack,
     0.0f, -0.015f, 0.0f, 6.2f, 0.03f, 5.2f},
    {WorldPrimitiveKind::Signage, MaterialRole::CrtCyan,
     -10.0f, 0.018f, 0.0f, 0.018f, 0.035f, 15.8f,
     0.0f, 0.0f, 0.0f, 11, 2.0f, 0.0f, 0.0f},
    {WorldPrimitiveKind::Signage, MaterialRole::SignalMagenta,
     0.0f, 0.019f, -8.0f, 19.8f, 0.036f, 0.018f,
     0.0f, 0.0f, 0.0f, 9, 0.0f, 0.0f, 2.0f},

    // Powder-concrete shell with deliberate missing edges.
    {WorldPrimitiveKind::Wall, MaterialRole::PowderConcrete,
     0.0f, 2.65f, 7.90f, 20.0f, 5.30f, 0.28f},
    {WorldPrimitiveKind::Wall, MaterialRole::PowderConcrete,
     -9.88f, 2.65f, 2.90f, 0.28f, 5.30f, 10.2f},
    {WorldPrimitiveKind::Wall, MaterialRole::PowderConcrete,
     9.88f, 2.65f, 5.25f, 0.28f, 5.30f, 5.3f},
    {WorldPrimitiveKind::Signage, MaterialRole::CrtCyan,
     0.0f, 0.18f, 7.68f, 19.4f, 0.12f, 0.10f},
    {WorldPrimitiveKind::Signage, MaterialRole::SignalMagenta,
     -9.62f, 0.18f, 2.90f, 0.10f, 0.12f, 9.65f},
    {WorldPrimitiveKind::Signage, MaterialRole::SignalMagenta,
     9.62f, 0.18f, 5.25f, 0.10f, 0.12f, 4.75f},
    {WorldPrimitiveKind::Signage, MaterialRole::SignalMagenta,
     0.0f, 4.75f, 7.65f, 13.5f, 0.10f, 0.10f},

    // Sparse warehouse illumination.
    {WorldPrimitiveKind::Signage, MaterialRole::CrtCyan,
     -4.3f, 4.35f, 2.7f, 3.6f, 0.08f, 0.10f},
    {WorldPrimitiveKind::Signage, MaterialRole::SodiumAmber,
     4.3f, 4.35f, 2.7f, 3.6f, 0.08f, 0.10f},

    // Table and embedded fictional terminal.
    {WorldPrimitiveKind::Casino, MaterialRole::IndustrialDark,
     0.0f, 0.52f, 0.0f, 0.72f, 1.04f, 0.72f},
    {WorldPrimitiveKind::Casino, MaterialRole::PowderConcrete,
     0.0f, 1.03f, 0.0f, 3.80f, 0.18f, 2.40f},
    {WorldPrimitiveKind::Casino, MaterialRole::TerminalGreen,
     0.0f, 1.14f, 0.0f, 3.35f, 0.05f, 1.95f},
    {WorldPrimitiveKind::Furniture, MaterialRole::IndustrialDark,
     0.0f, 0.36f, 1.73f, 0.90f, 0.18f, 0.90f},
    {WorldPrimitiveKind::Furniture, MaterialRole::IndustrialDark,
     0.0f, 0.72f, 2.05f, 0.90f, 0.72f, 0.18f},
    {WorldPrimitiveKind::Terminal, MaterialRole::VoidBlack,
     0.0f, 1.55f, -0.78f, 1.28f, 0.86f, 0.12f},
    {WorldPrimitiveKind::Terminal, MaterialRole::IndustrialDark,
     0.0f, 1.12f, -0.58f, 0.62f, 0.10f, 0.60f},

    // Sittable void couch assembled from repeatable furniture blocks.
    {WorldPrimitiveKind::Furniture, MaterialRole::IndustrialDark,
     5.72f, 0.40f, 4.63f, 3.20f, 0.55f, 1.55f},
    {WorldPrimitiveKind::Furniture, MaterialRole::SignalMagenta,
     5.72f, 1.10f, 5.20f, 3.20f, 1.20f, 0.38f},
    {WorldPrimitiveKind::Furniture, MaterialRole::CrtCyan,
     4.22f, 0.82f, 4.62f, 0.28f, 0.85f, 1.50f,
     0.0f, 0.0f, 0.0f, 2, 3.0f, 0.0f, 0.0f},
    {WorldPrimitiveKind::Furniture, MaterialRole::PowderConcrete,
     4.90f, 0.73f, 4.30f, 1.25f, 0.22f, 0.82f,
     0.0f, 0.0f, 0.0f, 2, 1.40f, 0.0f, 0.0f},
    {WorldPrimitiveKind::Furniture, MaterialRole::PowderConcrete,
     6.54f, 0.73f, 4.30f, 1.25f, 0.22f, 0.82f,
     0.0f, 0.0f, 0.0f, 2, 1.40f, 0.0f, 0.0f},

    // Mobility datum beside spawn: small physical silhouettes reinforce the
    // 2/3 mode HUD without turning the room into a vehicle showroom.
    {WorldPrimitiveKind::Mobility, MaterialRole::IndustrialDark,
     -6.10f, 0.10f, 4.55f, 4.90f, 0.20f, 2.20f},
    {WorldPrimitiveKind::Mobility, MaterialRole::SignalMagenta,
     -7.05f, 0.52f, 4.55f, 0.78f, 0.10f, 1.55f},
    {WorldPrimitiveKind::Mobility, MaterialRole::CrtCyan,
     -7.05f, 0.40f, 4.03f, 0.92f, 0.08f, 0.14f,
     0.0f, 0.0f, 0.0f, 2, 0.0f, 0.0f, 1.04f},
    {WorldPrimitiveKind::Mobility, MaterialRole::CrtCyan,
     -4.95f, 0.73f, 4.05f, 0.16f, 1.20f, 0.16f,
     0.0f, 0.0f, 0.0f, 2, 0.0f, 0.0f, 1.00f},
    {WorldPrimitiveKind::Mobility, MaterialRole::SignalMagenta,
     -4.95f, 0.74f, 4.55f, 0.14f, 0.14f, 1.25f,
     -0.45f, 0.0f, 0.0f},
    {WorldPrimitiveKind::Signage, MaterialRole::SodiumAmber,
     -8.28f, 1.10f, 4.55f, 0.12f, 2.20f, 0.12f,
     0.0f, 0.0f, 0.0f, 2, 6.36f, 0.0f, 0.0f},

    // Plinths and sparse CRT signage.
    {WorldPrimitiveKind::Monument, MaterialRole::IndustrialDark,
     -6.15f, 1.15f, 6.45f, 1.85f, 2.30f, 1.35f},
    {WorldPrimitiveKind::Signage, MaterialRole::CrtCyan,
     -6.15f, 1.65f, 5.74f, 1.35f, 0.82f, 0.08f},
    {WorldPrimitiveKind::Monument, MaterialRole::SodiumAmber,
     -8.55f, 1.8f, 6.75f, 0.36f, 3.60f, 0.36f,
     0.0f, 0.0f, 0.0f, 2, 17.1f, 0.0f, 0.0f},

    // Traversal grammar: a readable ramp into an elevated unfinished gallery.
    {WorldPrimitiveKind::Ramp, MaterialRole::PowderConcrete,
     -6.0f, 1.0f, 0.0f, 4.48f, 0.22f, 2.80f,
     0.0f, 0.0f, kRampAngle},
    {WorldPrimitiveKind::Platform, MaterialRole::PowderConcrete,
     -3.0f, 1.85f, 0.0f, 2.0f, 0.30f, 6.0f},
    {WorldPrimitiveKind::Signage, MaterialRole::HazardRed,
     -2.08f, 2.55f, 0.0f, 0.10f, 1.10f, 5.6f},
    {WorldPrimitiveKind::Signage, MaterialRole::CrtCyan,
     -3.0f, 2.10f, -2.75f, 1.75f, 0.08f, 0.08f},

    // v0.75 movement line: the plaza now advertises a readable sequence of
    // manual strip -> low rail -> transfer kicker. These remain reusable world
    // primitives; ride disciplines decide what each semantic volume means.
    {WorldPrimitiveKind::Floor, MaterialRole::VoidBlack,
     -1.00f, 0.018f, 4.72f, 5.60f, 0.036f, 1.30f},
    {WorldPrimitiveKind::Signage, MaterialRole::SodiumAmber,
     -1.00f, 0.044f, 4.72f, 5.25f, 0.030f, 0.08f,
     0.0f, 0.0f, 0.0f, 3, 0.0f, 0.0f, 0.52f},
    {WorldPrimitiveKind::Mobility, MaterialRole::CrtCyan,
     -0.85f, 0.34f, -2.42f, 5.20f, 0.10f, 0.10f},
    {WorldPrimitiveKind::Mobility, MaterialRole::IndustrialDark,
     -3.20f, 0.17f, -2.42f, 0.10f, 0.34f, 0.10f,
     0.0f, 0.0f, 0.0f, 3, 2.35f, 0.0f, 0.0f},
    {WorldPrimitiveKind::Signage, MaterialRole::SignalMagenta,
     -0.85f, 0.08f, -2.42f, 5.45f, 0.035f, 0.18f},
    {WorldPrimitiveKind::Ramp, MaterialRole::PowderConcrete,
     5.00f, 0.38f, 1.05f, 2.14f, 0.20f, 1.80f,
     0.0f, 0.0f, kKickerAngle},
    {WorldPrimitiveKind::Signage, MaterialRole::HazardRed,
     5.00f, 0.11f, 0.10f, 2.30f, 0.06f, 0.08f},

    // SPARRING DATUM: a readable semantic zone, not combat logic. The combat
    // layer interprets FightZone/SparAnchor; the renderer only draws markers.
    {WorldPrimitiveKind::Floor, MaterialRole::IndustrialDark,
     5.55f, 0.015f, -5.15f, 5.80f, 0.03f, 3.20f},
    {WorldPrimitiveKind::Signage, MaterialRole::HazardRed,
     5.55f, 0.045f, -6.70f, 5.80f, 0.05f, 0.08f,
     0.0f, 0.0f, 0.0f, 2, 0.0f, 0.0f, 3.10f},
    {WorldPrimitiveKind::Signage, MaterialRole::CrtCyan,
     2.70f, 0.047f, -5.15f, 0.08f, 0.05f, 3.10f,
     0.0f, 0.0f, 0.0f, 2, 5.70f, 0.0f, 0.0f},
    {WorldPrimitiveKind::Signage, MaterialRole::SodiumAmber,
     3.20f, 1.30f, -6.35f, 0.16f, 2.60f, 0.16f,
     0.0f, 0.0f, 0.0f, 2, 4.70f, 0.0f, 2.40f},
    {WorldPrimitiveKind::Signage, MaterialRole::HazardRed,
     5.55f, 0.055f, -5.15f, 0.10f, 0.055f, 2.45f},

    // Archaeological/digital monument: readable silhouette, unclear purpose.
    {WorldPrimitiveKind::Monument, MaterialRole::VoidBlack,
     6.70f, 2.10f, -3.90f, 1.50f, 4.20f, 1.50f},
    {WorldPrimitiveKind::Monument, MaterialRole::PowderConcrete,
     6.70f, 4.35f, -3.90f, 3.80f, 0.30f, 0.90f},
    {WorldPrimitiveKind::Monument, MaterialRole::SignalMagenta,
     6.70f, 2.30f, -3.08f, 0.82f, 2.10f, 0.06f},
    {WorldPrimitiveKind::Monument, MaterialRole::CrtCyan,
     6.70f, 5.20f, -3.90f, 0.24f, 1.50f, 0.24f},

    // Distant markers make negative space tangible without filling it.
    {WorldPrimitiveKind::VoidMarker, MaterialRole::CrtCyan,
     -11.5f, -2.0f, -8.8f, 0.16f, 1.20f, 0.16f,
     0.0f, 0.0f, 0.0f, 4, 6.2f, -0.7f, -1.7f},
    {WorldPrimitiveKind::VoidMarker, MaterialRole::SignalMagenta,
     -8.4f, -3.4f, -10.5f, 0.16f, 1.20f, 0.16f,
     0.0f, 0.0f, 0.0f, 4, 6.2f, 0.7f, -1.7f}
});

constexpr auto kSpecimenAffordances = std::to_array<WorldAffordanceVolume>({
    {1001, "FUSION TABLE SEAT",
     WorldAffordance::Seat | WorldAffordance::CasinoAnchor |
         WorldAffordance::Terminal,
     -0.75f, 0.75f, -0.50f, 2.50f, 1.25f, 2.45f,
     {0.0f, -0.34f, 1.72f, 3.14159265358979323846f},
     {0.0f, 0.0f, 2.35f, 3.14159265358979323846f}},
    {1002, "VOID COUCH",
     affordanceMask(WorldAffordance::Seat),
     4.0f, 7.4f, -0.50f, 2.50f, 3.0f, 5.6f,
     {5.72f, -0.34f, 3.92f, 3.14159265358979323846f},
     {5.72f, 0.0f, 3.05f, 3.14159265358979323846f}},
    {1101, "GALLERY RAMP",
     WorldAffordance::Rideable | WorldAffordance::Transition |
         WorldAffordance::Launch,
     -8.2f, -3.8f, -0.50f, 2.60f, -1.6f, 1.6f},
    {1102, "ELEVATED GALLERY",
     WorldAffordance::Landing | WorldAffordance::ManualZone |
         WorldAffordance::StallAnchor,
     -4.2f, -1.8f, 1.60f, 2.70f, -3.2f, 3.2f},
    {1103, "GALLERY EDGE RAIL",
     affordanceMask(WorldAffordance::Grindable),
     -2.30f, -1.90f, 1.80f, 3.20f, -2.9f, 2.9f,
     {-2.10f, 2.0f, 0.0f, 0.0f}},
    {1104, "PLAZA LOW RAIL",
     WorldAffordance::Grindable | WorldAffordance::StallAnchor,
     -3.55f, 1.85f, -0.25f, 1.45f, -2.78f, -2.06f,
     {-0.85f, 0.28f, -2.42f, 1.57079632679f}},
    {1105, "POWDER MANUAL STRIP",
     WorldAffordance::Landing | WorldAffordance::ManualZone,
     -3.85f, 1.85f, -0.25f, 1.20f, 3.92f, 5.50f},
    {1106, "TRANSFER KICKER",
     WorldAffordance::Rideable | WorldAffordance::Transition |
         WorldAffordance::Launch | WorldAffordance::Landing,
     3.65f, 6.35f, -0.30f, 2.20f, -0.05f, 2.10f},
    {1201, "SPARRING DATUM",
     WorldAffordance::FightZone | WorldAffordance::SparAnchor,
     2.40f, 8.80f, -0.20f, 3.00f, -7.20f, -1.60f,
     {4.35f, 0.0f, -5.15f, 1.57079632679f},
     {5.90f, 0.0f, -5.15f, -1.57079632679f}},
    {1202, "SPARRING SPECTATOR MARK",
     affordanceMask(WorldAffordance::SpectatorZone),
     2.0f, 9.2f, -0.20f, 3.00f, -1.60f, -0.40f},
    {1301, "SPAWN RECOVERY",
     affordanceMask(WorldAffordance::RespawnVolume),
     -1.5f, 1.5f, -0.20f, 3.00f, 4.3f, 6.3f},
    {1302, "BLACK SPACE",
     affordanceMask(WorldAffordance::Void),
     -1000.0f, 1000.0f, -1000.0f, -0.20f, -1000.0f, 1000.0f}
});

} // namespace

const std::array<HorizontalCollider, 5> BlackRoom::colliders_{{
    // Central Fusion table, couch, table-side data plinth, back wall, and a
    // partial left wall. The open front and right edge lead into black space.
    {-1.90f, 1.90f, -1.20f, 1.20f},
    {4.10f, 7.35f, 3.80f, 5.45f},
    {-7.15f, -5.15f, 5.65f, 7.30f},
    {-10.0f, 10.0f, 7.72f, 8.10f},
    {-10.0f, -9.62f, -2.25f, 8.10f}
}};

const std::array<WalkableSurface, 3> BlackRoom::surfaces_{{
    {-8.0f, -4.0f, -1.40f, 1.40f, 0.0f, 0.50f, 0.0f},
    {-4.0f, -2.0f, -3.00f, 3.00f, 2.0f, 0.0f, 0.0f},
    {4.0f, 6.0f, 0.15f, 1.95f, 0.0f, 0.38f, 0.0f}
}};

MovementEnvironment BlackRoom::movementEnvironment() const noexcept
{
    MovementEnvironment environment;
    environment.floorMinimumX = floorMinimumX;
    environment.floorMaximumX = floorMaximumX;
    environment.floorMinimumZ = floorMinimumZ;
    environment.floorMaximumZ = floorMaximumZ;
    environment.floorHeight = 0.0f;
    environment.voidResetHeight = -12.0f;
    environment.spawnX = 0.0f;
    environment.spawnY = 0.0f;
    environment.spawnZ = 5.25f;
    environment.colliders = colliders_;
    environment.surfaces = surfaces_;
    return environment;
}

std::span<const WorldPrimitive> BlackRoom::geometry() const noexcept
{
    return kSpecimenGeometry;
}

std::span<const WorldAffordanceVolume> BlackRoom::affordances() const noexcept
{
    return kSpecimenAffordances;
}

const WorldAffordanceVolume* BlackRoom::firstAffordance(
    WorldAffordance affordance
) const noexcept
{
    for (const WorldAffordanceVolume& volume : kSpecimenAffordances) {
        if (hasAffordance(volume.affordances, affordance)) {
            return &volume;
        }
    }
    return nullptr;
}

const WorldAffordanceVolume* BlackRoom::affordanceAt(
    WorldAffordance affordance,
    float x,
    float y,
    float z
) const noexcept
{
    for (const WorldAffordanceVolume& volume : kSpecimenAffordances) {
        if (hasAffordance(volume.affordances, affordance) &&
            volume.contains(x, y, z)) {
            return &volume;
        }
    }
    return nullptr;
}

const WorldAffordanceVolume* BlackRoom::affordanceById(
    std::uint32_t id
) const noexcept
{
    for (const WorldAffordanceVolume& volume : kSpecimenAffordances) {
        if (volume.id == id) {
            return &volume;
        }
    }
    return nullptr;
}

bool BlackRoom::hasAffordanceAt(
    WorldAffordance affordance,
    float x,
    float y,
    float z
) const noexcept
{
    return affordanceAt(affordance, x, y, z) != nullptr;
}

std::span<const SeatAnchor> BlackRoom::seatAnchors() const noexcept
{
    return seatAnchors_;
}

const SeatAnchor* BlackRoom::seatAnchorById(std::uint32_t id) const noexcept
{
    for (const SeatAnchor& anchor : seatAnchors_) {
        if (anchor.id == id) {
            return &anchor;
        }
    }
    return nullptr;
}

SeatAnchor* BlackRoom::mutableSeatAnchorById(std::uint32_t id) noexcept
{
    for (SeatAnchor& anchor : seatAnchors_) {
        if (anchor.id == id) {
            return &anchor;
        }
    }
    return nullptr;
}

const SeatAnchor* BlackRoom::nearestAvailableSeat(
    std::uint32_t furnitureAffordanceId,
    const PlayerState& player
) const noexcept
{
    const SeatAnchor* nearest = nullptr;
    float nearestDistanceSquared = std::numeric_limits<float>::max();
    for (const SeatAnchor& anchor : seatAnchors_) {
        if (anchor.furnitureAffordanceId == furnitureAffordanceId &&
            !anchor.occupied) {
            const ResolvedSeatAnchor resolved = resolvedSeatAnchor(anchor.id);
            const float dx = player.x - resolved.worldPosition.x;
            const float dz = player.z - resolved.worldPosition.z;
            const float distanceSquared = dx * dx + dz * dz;
            if (distanceSquared < nearestDistanceSquared) {
                nearest = &anchor;
                nearestDistanceSquared = distanceSquared;
            }
        }
    }
    return nearest;
}

ResolvedSeatAnchor BlackRoom::resolvedSeatAnchor(std::uint32_t id) const noexcept
{
    const SeatAnchor* seat = seatAnchorById(id);
    if (!seat) {
        return {};
    }
    const WorldAffordanceVolume* furniture = affordanceById(
        seat->furnitureAffordanceId
    );
    if (!furniture) {
        return {};
    }

    const float cosine = std::cos(furniture->primaryAnchor.yaw);
    const float sine = std::sin(furniture->primaryAnchor.yaw);
    WorldAnchor world;
    world.x = furniture->primaryAnchor.x +
        seat->localPosition.x * cosine + seat->localPosition.z * sine;
    world.y = furniture->primaryAnchor.y + seat->localPosition.y;
    world.z = furniture->primaryAnchor.z -
        seat->localPosition.x * sine + seat->localPosition.z * cosine;
    world.yaw = furniture->primaryAnchor.yaw + seat->localPosition.yaw;
    return {
        seat->id,
        seat->furnitureAffordanceId,
        world,
        seat->occupied,
        seat->poseProfile
    };
}

bool BlackRoom::seatOccupied(std::uint32_t id) const noexcept
{
    const SeatAnchor* seat = seatAnchorById(id);
    return seat && seat->occupied;
}

float BlackRoom::seatAlignmentError(const PlayerState& player) const noexcept
{
    if (!player.seatOccupancy || player.activeSeatAnchorId == 0) {
        return 0.0f;
    }
    const ResolvedSeatAnchor resolved = resolvedSeatAnchor(
        player.activeSeatAnchorId
    );
    if (resolved.id == 0) {
        return std::numeric_limits<float>::infinity();
    }
    const float dx = player.x - resolved.worldPosition.x;
    const float dy = player.y - resolved.worldPosition.y;
    const float dz = player.z - resolved.worldPosition.z;
    return std::sqrt(dx * dx + dy * dy + dz * dz);
}

RoomInteractionFocus BlackRoom::nearestInteraction(
    const PlayerState& player
) const noexcept
{
    if (player.activity != PlayerActivity::Roaming || !player.grounded) {
        return {};
    }

    RoomInteractionFocus nearest;
    float nearestDistance = std::numeric_limits<float>::max();
    for (const WorldAffordanceVolume& volume : kSpecimenAffordances) {
        if (!hasAffordance(volume.affordances, WorldAffordance::Seat)) {
            continue;
        }
        const SeatAnchor* seat = nearestAvailableSeat(volume.id, player);
        if (!seat) {
            continue;
        }
        const float dx = player.x - volume.secondaryAnchor.x;
        const float dz = player.z - volume.secondaryAnchor.z;
        const float distance = std::sqrt(dx * dx + dz * dz);
        constexpr float interactionRadius = 1.50f;
        if (distance <= interactionRadius && distance < nearestDistance) {
            const bool casino = hasAffordance(
                volume.affordances,
                WorldAffordance::CasinoAnchor
            );
            nearest = {
                casino
                    ? RoomInteractionKind::FusionTable
                    : RoomInteractionKind::LoungeCouch,
                volume.id,
                seat->id,
                distance,
                casino
                    ? std::string_view{"SIT // FUSION TABLE"}
                    : std::string_view{"SIT // VOID COUCH"}
            };
            nearestDistance = distance;
        }
    }
    return nearest;
}

bool BlackRoom::engageNearest(PlayerState& player) noexcept
{
    const RoomInteractionFocus focus = nearestInteraction(player);
    if (!focus) {
        return false;
    }

    const WorldAffordanceVolume* volume = affordanceById(focus.affordanceId);
    SeatAnchor* seat = mutableSeatAnchorById(focus.seatAnchorId);
    if (!volume || !seat || seat->occupied ||
        seat->furnitureAffordanceId != volume->id) {
        return false;
    }
    const ResolvedSeatAnchor resolved = resolvedSeatAnchor(seat->id);
    if (resolved.id == 0) {
        return false;
    }

    seat->occupied = true;
    player.velocityX = 0.0f;
    player.velocityY = 0.0f;
    player.velocityZ = 0.0f;
    player.grounded = true;
    player.movementBlend = 0.0f;
    player.activeAffordanceId = focus.affordanceId;
    player.activeSeatAnchorId = seat->id;
    player.seatAnchorError = 0.0f;
    player.seatOccupancy = true;

    player.x = resolved.worldPosition.x;
    player.y = resolved.worldPosition.y;
    player.z = resolved.worldPosition.z;
    player.yaw = resolved.worldPosition.yaw;

    switch (focus.kind) {
        case RoomInteractionKind::FusionTable:
            player.activity = PlayerActivity::CasinoSeated;
            return true;

        case RoomInteractionKind::LoungeCouch:
            player.activity = PlayerActivity::CouchSeated;
            return true;

        case RoomInteractionKind::None:
            break;
    }
    seat->occupied = false;
    player.activeAffordanceId = 0;
    player.activeSeatAnchorId = 0;
    player.seatOccupancy = false;
    return false;
}

bool BlackRoom::leaveInteraction(PlayerState& player) noexcept
{
    if (player.activity == PlayerActivity::Roaming) {
        return false;
    }

    const WorldAffordanceVolume* volume = affordanceById(
        player.activeAffordanceId
    );
    if (!volume) {
        return false;
    }

    if (SeatAnchor* seat = mutableSeatAnchorById(player.activeSeatAnchorId)) {
        seat->occupied = false;
    }

    player.x = volume->secondaryAnchor.x;
    player.y = volume->secondaryAnchor.y;
    player.z = volume->secondaryAnchor.z;
    player.yaw = volume->secondaryAnchor.yaw;
    player.velocityX = 0.0f;
    player.velocityY = 0.0f;
    player.velocityZ = 0.0f;
    player.grounded = true;
    player.activity = PlayerActivity::Roaming;
    player.activeAffordanceId = 0;
    player.activeSeatAnchorId = 0;
    player.seatAnchorError = 0.0f;
    player.seatOccupancy = false;
    return true;
}

} // namespace hakui
