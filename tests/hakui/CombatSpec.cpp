#include <cassert>
#include <cmath>

#include "combat/CombatSimulation.hpp"

namespace {

using namespace hakui::combat;

constexpr CombatZone kSparZone{
    1201,
    {4.35f, 0.0f, -3.90f},
    {5.85f, 0.0f, -3.90f}
};

constexpr CombatFrameContext kInRange{
    kSparZone.playerAnchor,
    kSparZone.opponentAnchor
};

CombatFrameResult stepUntilDamage(
    CombatSimulation& simulation,
    CombatIntent player,
    CombatIntent opponent = {}
)
{
    (void)simulation.update(player, opponent, kInRange, 0.01f);
    CombatFrameResult result;
    for (int frame = 0; frame < 40; ++frame) {
        result = simulation.update({}, {}, kInRange, 0.02f);
        if (result.damageEventCount > 0) {
            return result;
        }
    }
    return result;
}

} // namespace

int main()
{
    using namespace hakui::combat;

    CombatSimulation combat;
    assert(!combat.active());
    assert(combat.enter(kSparZone));
    assert(combat.active());
    assert(combat.player().state == CombatState::Ready);
    assert(combat.player().targetEntity == combat.opponent().entity);

    CombatIntent footwork;
    footwork.moveForward = 1.0f;
    footwork.moveRight = 0.5f;
    (void)combat.update(footwork, {}, kInRange, 0.10f);
    assert(combat.player().footworkForward > 0.0f);
    assert(combat.player().footworkLateral > 0.0f);
    assert(combat.player().stanceBlend > 0.0f);
    assert(combat.playerWorldPosition().x > kInRange.playerPosition.x);
    assert(combat.playerWorldPosition().z > kInRange.playerPosition.z);

    const float initialOpponentHealth = combat.opponent().health;
    const CombatFrameResult jab = stepUntilDamage(
        combat,
        {AttackSemantic::Jab, DefenseIntent::None, false}
    );
    assert(jab.damageEventCount == 1);
    assert(jab.damageEvents[0].discipline == CombatDiscipline::Unarmed);
    assert(jab.damageEvents[0].attack == AttackSemantic::Jab);
    assert(jab.damageEvents[0].result == HitResult::Hit);
    assert(jab.damageEvents[0].source == combat.player().entity);
    assert(jab.damageEvents[0].target == combat.opponent().entity);
    assert(jab.damageEvents[0].hitPosition.y > 1.0f);
    assert(jab.damageEvents[0].impactDirection.x > 0.0f);
    assert(combat.opponent().health < initialOpponentHealth);
    assert(combat.player().stamina < 100.0f);

    combat.leave();
    assert(!combat.active());
    assert(combat.player().state == CombatState::Inactive);
    assert(combat.enter(kSparZone));

    CombatFrameContext farApart = kInRange;
    farApart.opponentPosition.x += 8.0f;
    (void)combat.update(
        {AttackSemantic::Cross, DefenseIntent::None, false},
        {},
        farApart,
        0.01f
    );
    CombatFrameResult miss;
    for (int frame = 0; frame < 40 && miss.damageEventCount == 0; ++frame) {
        miss = combat.update({}, {}, farApart, 0.02f);
    }
    assert(miss.damageEventCount == 1);
    assert(miss.damageEvents[0].result == HitResult::Miss);
    assert(std::fabs(combat.opponent().health - 100.0f) < 0.001f);

    combat.leave();
    assert(combat.enter(kSparZone));
    (void)combat.update({}, {AttackSemantic::Cross, DefenseIntent::None, false},
                        kInRange, 0.01f);
    CombatFrameResult guarded;
    for (int frame = 0; frame < 40 && guarded.damageEventCount == 0; ++frame) {
        guarded = combat.update(
            {AttackSemantic::None, DefenseIntent::Guard, false},
            {},
            kInRange,
            0.02f
        );
    }
    assert(guarded.damageEventCount == 1);
    assert(guarded.damageEvents[0].result == HitResult::Guarded);
    assert(guarded.damageEvents[0].damage < 10.0f);

    combat.leave();
    assert(combat.enter(kSparZone));
    combat.playerForTesting().balance = 10.0f;
    const CombatFrameResult knockdown = stepUntilDamage(
        combat,
        {},
        {AttackSemantic::Cross, DefenseIntent::None, false}
    );
    assert(knockdown.playerKnockedDown);
    assert(combat.player().state == CombatState::KnockedDown);
    for (int frame = 0; frame < 10; ++frame) {
        (void)combat.update({}, {}, kInRange, 0.10f);
    }
    const CombatFrameResult recovered = combat.update(
        {AttackSemantic::None, DefenseIntent::None, true},
        {},
        kInRange,
        0.02f
    );
    assert(recovered.playerRecovered);
    assert(combat.player().state == CombatState::Ready);

    const AttackDefinition swordStub = CombatSimulation::attackDefinition(
        CombatDiscipline::Sword,
        AttackSemantic::SlashLeft
    );
    const AttackDefinition bowStub = CombatSimulation::attackDefinition(
        CombatDiscipline::Bow,
        AttackSemantic::ArrowRelease
    );
    assert(!swordStub.enabled);
    assert(!bowStub.enabled);

    combat.playerForTesting().equipment.discipline = CombatDiscipline::Sword;
    combat.playerForTesting().stamina = 100.0f;
    const float swordStamina = combat.player().stamina;
    const CombatFrameResult disabledSword = combat.update(
        {AttackSemantic::SlashLeft, DefenseIntent::None, false},
        {},
        kInRange,
        0.02f
    );
    assert(!disabledSword.playerAttackStarted);
    assert(combat.player().state == CombatState::Ready);
    assert(std::fabs(combat.player().stamina - swordStamina) < 0.001f);

    combat.leave();
    return 0;
}
