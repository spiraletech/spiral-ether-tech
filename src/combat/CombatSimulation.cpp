#include "combat/CombatSimulation.hpp"

#include <algorithm>
#include <cmath>

namespace hakui::combat {

namespace {

constexpr float kMaximumVital = 100.0f;
constexpr float kStaminaRecoveryPerSecond = 18.0f;
constexpr float kGuardStaminaPerSecond = 14.0f;
constexpr float kKnockdownRecoveryDelay = 0.85f;

float distanceBetween(const CombatVector& left, const CombatVector& right) noexcept
{
    const float x = right.x - left.x;
    const float y = right.y - left.y;
    const float z = right.z - left.z;
    return std::sqrt(x * x + y * y + z * z);
}

CombatVector directionBetween(
    const CombatVector& source,
    const CombatVector& target
) noexcept
{
    const float distance = distanceBetween(source, target);
    if (distance <= 0.0001f) {
        return {0.0f, 0.0f, 1.0f};
    }
    return {
        (target.x - source.x) / distance,
        (target.y - source.y) / distance,
        (target.z - source.z) / distance
    };
}

void resetCombatant(CombatantState& actor, EntityId target) noexcept
{
    actor.targetEntity = target;
    actor.state = CombatState::Ready;
    actor.pendingAttack = AttackSemantic::None;
    actor.health = kMaximumVital;
    actor.stamina = kMaximumVital;
    actor.balance = kMaximumVital;
    actor.phaseSeconds = 0.0f;
    actor.releaseSeconds = 0.0f;
    actor.recoverySeconds = 0.0f;
    actor.footworkForward = 0.0f;
    actor.footworkLateral = 0.0f;
    actor.stanceBlend = 0.0f;
    actor.impactSeconds = 0.0f;
}

} // namespace

const DamageEvent* CombatFrameResult::firstDamageTo(EntityId target) const noexcept
{
    for (std::size_t index = 0; index < damageEventCount; ++index) {
        if (damageEvents[index].target == target) {
            return &damageEvents[index];
        }
    }
    return nullptr;
}

CombatSimulation::CombatSimulation(EntityId playerEntity, EntityId opponentEntity)
{
    player_.entity = playerEntity;
    player_.equipment.discipline = CombatDiscipline::Unarmed;
    opponent_.entity = opponentEntity;
    opponent_.equipment.discipline = CombatDiscipline::Unarmed;
}

bool CombatSimulation::enter(const CombatZone& zone) noexcept
{
    if (active_ || zone.worldAffordanceId == 0) {
        return false;
    }

    zone_ = zone;
    resetCombatant(player_, opponent_.entity);
    resetCombatant(opponent_, player_.entity);
    playerWorldPosition_ = zone.playerAnchor;
    opponentWorldPosition_ = zone.opponentAnchor;
    active_ = true;
    return true;
}

void CombatSimulation::leave() noexcept
{
    active_ = false;
    player_.state = CombatState::Inactive;
    player_.pendingAttack = AttackSemantic::None;
    opponent_.state = CombatState::Inactive;
    opponent_.pendingAttack = AttackSemantic::None;
}

bool CombatSimulation::active() const noexcept
{
    return active_;
}

const CombatZone& CombatSimulation::zone() const noexcept
{
    return zone_;
}

const CombatantState& CombatSimulation::player() const noexcept
{
    return player_;
}

const CombatantState& CombatSimulation::opponent() const noexcept
{
    return opponent_;
}

const CombatVector& CombatSimulation::playerWorldPosition() const noexcept
{
    return playerWorldPosition_;
}

const CombatVector& CombatSimulation::opponentWorldPosition() const noexcept
{
    return opponentWorldPosition_;
}

CombatantState& CombatSimulation::playerForTesting() noexcept
{
    return player_;
}

CombatantState& CombatSimulation::opponentForTesting() noexcept
{
    return opponent_;
}

AttackDefinition CombatSimulation::attackDefinition(
    CombatDiscipline discipline,
    AttackSemantic semantic
) noexcept
{
    if (discipline == CombatDiscipline::Unarmed) {
        switch (semantic) {
            case AttackSemantic::Jab:
                return {discipline, semantic, 10.0f, 6.0f, 10.0f, 16.0f,
                        2.05f, 0.10f, 0.08f, 0.22f, true};
            case AttackSemantic::Cross:
                return {discipline, semantic, 18.0f, 10.0f, 16.0f, 28.0f,
                        2.15f, 0.18f, 0.10f, 0.34f, true};
            default:
                break;
        }
    }

    // Directional sword and projectile semantics are intentionally declared
    // but disabled through v0.8. Enabling them later means adding discipline
    // profiles, not replacing shared encounter, hit, or damage machinery.
    return {discipline, semantic};
}

CombatSimulation::PendingAttack CombatSimulation::advance(
    CombatantState& actor,
    CombatantState& target,
    const CombatIntent& intent,
    float deltaSeconds,
    bool actorIsPlayer,
    CombatFrameResult& result
) noexcept
{
    PendingAttack pending;
    if (!active_ || deltaSeconds <= 0.0f) {
        return pending;
    }

    if (actor.state == CombatState::KnockedDown) {
        actor.stanceBlend = 0.0f;
        actor.phaseSeconds = std::max(0.0f, actor.phaseSeconds - deltaSeconds);
        if (actor.phaseSeconds <= 0.0f && intent.recover) {
            actor.state = CombatState::Ready;
            actor.health = std::max(actor.health, 35.0f);
            actor.balance = 55.0f;
            actor.stamina = std::max(actor.stamina, 40.0f);
            if (actorIsPlayer) {
                result.playerRecovered = true;
            } else {
                result.opponentRecovered = true;
            }
        }
        return pending;
    }

    if (actor.state == CombatState::Staggered ||
        actor.state == CombatState::Recovery) {
        actor.phaseSeconds = std::max(0.0f, actor.phaseSeconds - deltaSeconds);
        if (actor.phaseSeconds <= 0.0f) {
            actor.state = CombatState::Ready;
        }
        return pending;
    }

    if (actor.state == CombatState::Release) {
        actor.phaseSeconds = std::max(0.0f, actor.phaseSeconds - deltaSeconds);
        if (actor.phaseSeconds <= 0.0f) {
            actor.state = CombatState::Recovery;
            actor.phaseSeconds = actor.recoverySeconds;
            actor.pendingAttack = AttackSemantic::None;
        }
        return pending;
    }

    if (actor.state == CombatState::Windup) {
        actor.phaseSeconds = std::max(0.0f, actor.phaseSeconds - deltaSeconds);
        if (actor.phaseSeconds <= 0.0f) {
            const AttackDefinition definition = attackDefinition(
                actor.equipment.discipline,
                actor.pendingAttack
            );
            if (definition.enabled) {
                actor.state = CombatState::Release;
                actor.phaseSeconds = actor.releaseSeconds;
                pending = {&actor, &target, definition, actorIsPlayer};
            } else {
                actor.state = CombatState::Ready;
                actor.pendingAttack = AttackSemantic::None;
            }
        }
        return pending;
    }

    if (intent.attack != AttackSemantic::None) {
        const AttackDefinition definition = attackDefinition(
            actor.equipment.discipline,
            intent.attack
        );
        if (definition.enabled && actor.stamina >= definition.staminaCost) {
            actor.stamina -= definition.staminaCost;
            actor.state = CombatState::Windup;
            actor.pendingAttack = intent.attack;
            actor.phaseSeconds = definition.windupSeconds;
            actor.releaseSeconds = definition.releaseSeconds;
            actor.recoverySeconds = definition.recoverySeconds;
            if (actorIsPlayer) {
                result.playerAttackStarted = true;
            } else {
                result.opponentAttackStarted = true;
            }
            return pending;
        }
    }

    const bool guarding = intent.defense == DefenseIntent::Guard &&
        actor.stamina > 0.0f;
    if (guarding) {
        actor.state = CombatState::Guarding;
        actor.stamina = std::max(
            0.0f,
            actor.stamina - kGuardStaminaPerSecond * deltaSeconds
        );
    } else {
        actor.state = CombatState::Ready;
        actor.stamina = std::min(
            kMaximumVital,
            actor.stamina + kStaminaRecoveryPerSecond * deltaSeconds
        );
    }
    return pending;
}

void CombatSimulation::resolve(
    const PendingAttack& attack,
    const CombatVector& sourcePosition,
    const CombatVector& targetPosition,
    CombatFrameResult& result
) noexcept
{
    if (!attack.source || !attack.target ||
        attack.target->state == CombatState::KnockedDown ||
        result.damageEventCount >= result.damageEvents.size()) {
        return;
    }

    DamageEvent event;
    event.source = attack.source->entity;
    event.target = attack.target->entity;
    event.discipline = attack.definition.discipline;
    event.attack = attack.definition.semantic;
    event.impactDirection = directionBetween(sourcePosition, targetPosition);
    event.hitPosition = {
        targetPosition.x,
        targetPosition.y + 1.55f,
        targetPosition.z
    };

    if (distanceBetween(sourcePosition, targetPosition) > attack.definition.reach) {
        event.result = HitResult::Miss;
        result.damageEvents[result.damageEventCount++] = event;
        return;
    }

    const bool guarded = attack.target->state == CombatState::Guarding;
    const float guardScale = guarded ? 0.20f : 1.0f;
    event.damage = attack.definition.damage * guardScale;
    event.stagger = attack.definition.stagger * (guarded ? 0.25f : 1.0f);
    event.knockdownPotential =
        attack.definition.knockdownPotential * (guarded ? 0.20f : 1.0f);

    attack.target->health = std::max(0.0f, attack.target->health - event.damage);
    attack.target->balance = std::max(
        0.0f,
        attack.target->balance - event.knockdownPotential
    );

    if (attack.target->health <= 0.0f || attack.target->balance <= 0.0f) {
        attack.target->state = CombatState::KnockedDown;
        attack.target->phaseSeconds = kKnockdownRecoveryDelay;
        attack.target->pendingAttack = AttackSemantic::None;
        event.result = HitResult::Knockdown;
        if (attack.sourceIsPlayer) {
            result.opponentKnockedDown = true;
        } else {
            result.playerKnockedDown = true;
        }
    } else if (guarded) {
        event.result = HitResult::Guarded;
    } else {
        attack.target->state = CombatState::Staggered;
        attack.target->phaseSeconds = std::clamp(
            event.stagger * 0.012f,
            0.16f,
            0.42f
        );
        attack.target->impactSeconds = attack.target->phaseSeconds;
        attack.target->pendingAttack = AttackSemantic::None;
        event.result = HitResult::Hit;
    }

    result.damageEvents[result.damageEventCount++] = event;
}

void CombatSimulation::updateFootwork(
    CombatantState& actor,
    const CombatIntent& intent,
    float deltaSeconds,
    bool actorIsPlayer
) noexcept
{
    actor.impactSeconds = std::max(0.0f, actor.impactSeconds - deltaSeconds);
    const bool mobile = actor.state == CombatState::Ready ||
        actor.state == CombatState::Guarding;
    const float forward = mobile && std::isfinite(intent.moveForward)
        ? std::clamp(intent.moveForward, -1.0f, 1.0f)
        : 0.0f;
    const float right = mobile && std::isfinite(intent.moveRight)
        ? std::clamp(intent.moveRight, -1.0f, 1.0f)
        : 0.0f;
    const float guardScale = actor.state == CombatState::Guarding ? 0.55f : 1.0f;
    actor.footworkForward = std::clamp(
        actor.footworkForward + forward * 1.55f * guardScale * deltaSeconds,
        -0.60f,
        actorIsPlayer ? 0.42f : 0.30f
    );
    actor.footworkLateral = std::clamp(
        actor.footworkLateral + right * 1.35f * guardScale * deltaSeconds,
        -0.68f,
        0.68f
    );
    const float targetBlend = std::sqrt(forward * forward + right * right);
    const float blendStep = 7.5f * deltaSeconds;
    if (actor.stanceBlend < targetBlend) {
        actor.stanceBlend = std::min(actor.stanceBlend + blendStep, targetBlend);
    } else {
        actor.stanceBlend = std::max(actor.stanceBlend - blendStep, targetBlend);
    }
}

void CombatSimulation::updateWorldPositions(
    const CombatFrameContext& context
) noexcept
{
    const CombatVector direction = directionBetween(
        context.playerPosition,
        context.opponentPosition
    );
    const CombatVector lateral{-direction.z, 0.0f, direction.x};
    playerWorldPosition_ = {
        context.playerPosition.x + direction.x * player_.footworkForward +
            lateral.x * player_.footworkLateral,
        context.playerPosition.y,
        context.playerPosition.z + direction.z * player_.footworkForward +
            lateral.z * player_.footworkLateral
    };
    opponentWorldPosition_ = {
        context.opponentPosition.x - direction.x * opponent_.footworkForward +
            lateral.x * opponent_.footworkLateral,
        context.opponentPosition.y,
        context.opponentPosition.z - direction.z * opponent_.footworkForward +
            lateral.z * opponent_.footworkLateral
    };
}

CombatFrameResult CombatSimulation::update(
    const CombatIntent& playerIntent,
    const CombatIntent& opponentIntent,
    const CombatFrameContext& context,
    float deltaSeconds
) noexcept
{
    CombatFrameResult result;
    if (!active_) {
        return result;
    }

    const float dt = std::clamp(deltaSeconds, 0.0f, 0.10f);
    updateFootwork(player_, playerIntent, dt, true);
    updateFootwork(opponent_, opponentIntent, dt, false);
    updateWorldPositions(context);
    const PendingAttack playerAttack = advance(
        player_, opponent_, playerIntent, dt, true, result
    );
    const PendingAttack opponentAttack = advance(
        opponent_, player_, opponentIntent, dt, false, result
    );

    // Both pending releases are captured before damage is resolved, allowing a
    // deterministic simultaneous trade while keeping all hit decisions here.
    resolve(
        playerAttack,
        playerWorldPosition_,
        opponentWorldPosition_,
        result
    );
    resolve(
        opponentAttack,
        opponentWorldPosition_,
        playerWorldPosition_,
        result
    );
    return result;
}

} // namespace hakui::combat
