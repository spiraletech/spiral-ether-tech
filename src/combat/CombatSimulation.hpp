#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace hakui::combat {

using EntityId = std::uint32_t;

enum class CombatDiscipline : std::uint8_t {
    Unarmed,
    Sword,
    Bow
};

enum class CombatState : std::uint8_t {
    Inactive,
    Ready,
    Guarding,
    Windup,
    Release,
    Recovery,
    Staggered,
    KnockedDown
};

// Semantics describe authored intent. A discipline interpreter decides whether
// the equipped discipline currently understands that intent.
enum class AttackSemantic : std::uint8_t {
    None,
    Jab,
    Cross,
    SlashLeft,
    SlashRight,
    Overhead,
    Thrust,
    ArrowRelease
};

enum class DefenseIntent : std::uint8_t {
    None,
    Guard,
    Block,
    Parry
};

enum class HitResult : std::uint8_t {
    None,
    Miss,
    Guarded,
    Hit,
    Knockdown
};

struct CombatVector {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct EquipmentState {
    CombatDiscipline discipline = CombatDiscipline::Unarmed;
    std::uint32_t equippedItem = 0;
};

struct CombatIntent {
    AttackSemantic attack = AttackSemantic::None;
    DefenseIntent defense = DefenseIntent::None;
    bool recover = false;
    float moveForward = 0.0f;
    float moveRight = 0.0f;
};

struct AttackDefinition {
    CombatDiscipline discipline = CombatDiscipline::Unarmed;
    AttackSemantic semantic = AttackSemantic::None;
    float staminaCost = 0.0f;
    float damage = 0.0f;
    float stagger = 0.0f;
    float knockdownPotential = 0.0f;
    float reach = 0.0f;
    float windupSeconds = 0.0f;
    float releaseSeconds = 0.0f;
    float recoverySeconds = 0.0f;
    bool enabled = false;
};

struct DamageEvent {
    EntityId source = 0;
    EntityId target = 0;
    CombatDiscipline discipline = CombatDiscipline::Unarmed;
    AttackSemantic attack = AttackSemantic::None;
    float damage = 0.0f;
    CombatVector impactDirection{};
    CombatVector hitPosition{};
    float stagger = 0.0f;
    float knockdownPotential = 0.0f;
    HitResult result = HitResult::None;
};

struct CombatantState {
    EntityId entity = 0;
    EntityId targetEntity = 0;
    EquipmentState equipment{};
    CombatState state = CombatState::Inactive;
    AttackSemantic pendingAttack = AttackSemantic::None;
    float health = 100.0f;
    float stamina = 100.0f;
    float balance = 100.0f;
    float phaseSeconds = 0.0f;
    float releaseSeconds = 0.0f;
    float recoverySeconds = 0.0f;
    float footworkForward = 0.0f;
    float footworkLateral = 0.0f;
    float stanceBlend = 0.0f;
    float impactSeconds = 0.0f;
};

struct CombatZone {
    std::uint32_t worldAffordanceId = 0;
    CombatVector playerAnchor{};
    CombatVector opponentAnchor{};
};

struct CombatFrameContext {
    CombatVector playerPosition{};
    CombatVector opponentPosition{};
};

struct CombatFrameResult {
    std::array<DamageEvent, 4> damageEvents{};
    std::size_t damageEventCount = 0;
    bool playerAttackStarted = false;
    bool opponentAttackStarted = false;
    bool playerRecovered = false;
    bool opponentRecovered = false;
    bool playerKnockedDown = false;
    bool opponentKnockedDown = false;

    const DamageEvent* firstDamageTo(EntityId target) const noexcept;
};

// One deterministic encounter state machine. It owns shared combat timing,
// targeting, stamina, hit/damage, interruption, knockdown, and recovery. The
// attack-definition interpreter below is the only discipline-specific seam.
class CombatSimulation {
public:
    CombatSimulation(EntityId playerEntity = 1, EntityId opponentEntity = 2);

    bool enter(const CombatZone& zone) noexcept;
    void leave() noexcept;
    bool active() const noexcept;

    CombatFrameResult update(
        const CombatIntent& playerIntent,
        const CombatIntent& opponentIntent,
        const CombatFrameContext& context,
        float deltaSeconds
    ) noexcept;

    const CombatZone& zone() const noexcept;
    const CombatantState& player() const noexcept;
    const CombatantState& opponent() const noexcept;
    const CombatVector& playerWorldPosition() const noexcept;
    const CombatVector& opponentWorldPosition() const noexcept;
    CombatantState& playerForTesting() noexcept;
    CombatantState& opponentForTesting() noexcept;

    static AttackDefinition attackDefinition(
        CombatDiscipline discipline,
        AttackSemantic semantic
    ) noexcept;

private:
    struct PendingAttack {
        CombatantState* source = nullptr;
        CombatantState* target = nullptr;
        AttackDefinition definition{};
        bool sourceIsPlayer = false;
    };

    PendingAttack advance(
        CombatantState& actor,
        CombatantState& target,
        const CombatIntent& intent,
        float deltaSeconds,
        bool actorIsPlayer,
        CombatFrameResult& result
    ) noexcept;

    void resolve(
        const PendingAttack& attack,
        const CombatVector& sourcePosition,
        const CombatVector& targetPosition,
        CombatFrameResult& result
    ) noexcept;
    void updateFootwork(
        CombatantState& actor,
        const CombatIntent& intent,
        float deltaSeconds,
        bool actorIsPlayer
    ) noexcept;
    void updateWorldPositions(const CombatFrameContext& context) noexcept;

    CombatantState player_{};
    CombatantState opponent_{};
    CombatZone zone_{};
    CombatVector playerWorldPosition_{};
    CombatVector opponentWorldPosition_{};
    bool active_ = false;
};

} // namespace hakui::combat
